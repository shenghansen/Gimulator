#include "communicate.h"
#include "mpi.h"
#include <cstddef>
#include <cstdint>
#include <omp.h>
#include <thread>
#define SIZE 2048
#define NUMA 4
#define PARTITIONS 2

int main(int argc, char** argv) {

    /* single process test */
    //     CXL_SHM cxl(1, 0);
    //     GIM_comm comm(cxl);
    //     uint8_t*** send_buffer[PARTITIONS];
    //     uint8_t*** recv_buffer[PARTITIONS];
    //     // send_buffer=new uint8_t**[PARTITIONS];
    //     for (size_t i = 0; i < PARTITIONS; i++) {
    //         send_buffer[i] = (uint8_t***)cxl->GIM_malloc(PARTITIONS * sizeof(uint8_t**), i);
    //         recv_buffer[i] = (uint8_t***)cxl->GIM_malloc(PARTITIONS * sizeof(uint8_t**), i);
    //         for (size_t j = 0; j < PARTITIONS; j++) {
    //             // send_buffer[i]=new uint8_t*[NUMA];
    //             send_buffer[i][j] = (uint8_t**)cxl->GIM_malloc(NUMA * sizeof(uint8_t*), i);
    //             recv_buffer[i][j] = (uint8_t**)cxl->GIM_malloc(NUMA * sizeof(uint8_t*), i);
    //             for (size_t k = 0; k < SIZE; k++) {
    //                 send_buffer[i][j][k] = cxl->GIM_malloc(SIZE, i);
    //                 recv_buffer[i][j][k] = cxl->GIM_malloc(SIZE, i);
    //             }
    //         }
    //     }
    //     for (int step = 0; step < SIZE; step++) {
    //         send_buffer[0][0][0][step] = step % 256;
    //         recv_buffer[0][0][0][step] = 0;
    //     }
    //     std::thread send_thread(
    //         [&]() { comm.GIM_Send(send_buffer[0][0][0], SIZE, 0, 0, recv_buffer[0]); });
    //     std::thread recv_thread([&]() { comm.GIM_Recv(0, SIZE, 0, 0); });
    //     send_thread.join();
    //     recv_thread.join();
    // #pragma omp parallel for
    //     for (int j = 0; j < SIZE; j++) {
    //         if (recv_buffer[0][0][0][j] != j % 256) {
    //             printf("%d\n", recv_buffer[0][0][0][j]);
    //         }
    //     }


    /* multi process test */
    MPI_Init(&argc, &argv);   // 初始化 MPI 环境
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);   // 获取进程总数
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);   // 获取当前进程编号
    printf("word_size %d\n", world_size);

    CXL_SHM* cxl = new CXL_SHM(world_size, world_rank);
    GIM_comm comm(cxl);


    //     size_t* send_buffer[world_size];
    //     for (size_t i = 0; i < world_size; i++) {
    //         send_buffer[i] = (size_t*)cxl->GIM_malloc(SIZE * sizeof(uint8_t), i);
    //     }
    //     for (size_t i = 0; i < world_size; i++)
    //     {
    //         for (size_t j = 0; j < SIZE; j++)
    //         {
    //            send_buffer[i][j]=0;
    //         }

    //     }

    // if (world_rank==0)
    // {

    //     for (size_t step = 0; step < world_size; step++) {
    // #pragma omp parallel for
    //         for (size_t i = 0; i < SIZE; i++) {
    //             send_buffer[step][i] = i ;
    //         }
    //     }
    // }

    //     MPI_Barrier(MPI_COMM_WORLD);
    //     if (world_rank==0)
    //     {
    //         size_t count = 0;
    //         for (int step = 1; step < world_size; step++) {   //遍历host
    //             int i = (world_rank + step) % world_size;
    //             // #pragma omp parallel for
    //             for (size_t j = 0; j < SIZE; j++) {
    //                 if (send_buffer[i][j] != j ) {
    //                     printf("%d,%d\n", j, send_buffer[i][j]);
    //                     count++;
    //                 }
    //             }
    //         }
    //         printf("partition: %d,fault count:%ld\n", world_rank, count);
    //     }
    //     MPI_Barrier(MPI_COMM_WORLD);
    //     if (world_rank == 1) {
    //         size_t count = 0;
    //         for (int step = 1; step < world_size; step++) {   //遍历host
    //             int i = (world_rank + step) % world_size;
    //             // #pragma omp parallel for
    //             for (size_t j = 0; j < SIZE; j++) {
    //                 if (send_buffer[i][j] != j ) {
    //                     printf("%d,%d\n", j, send_buffer[i][j]);
    //                     count++;
    //                 }
    //             }
    //         }
    //         printf("partition: %d,fault count:%ld\n", world_rank, count);
    //     }





    
    size_t*** send_buffer[world_size];
    size_t*** recv_buffer[world_size];
    for (size_t i = 0; i < world_size; i++) {
        send_buffer[i] = new size_t**[world_size];
        recv_buffer[i] = new size_t**[world_size];
        for (size_t j = 0; j < world_size; j++) {
            send_buffer[i][j] = new size_t*[NUMA];
            recv_buffer[i][j] = new size_t*[NUMA];
            for (size_t k = 0; k < NUMA; k++) {
                send_buffer[i][j][k] = (size_t*)cxl->GIM_malloc(SIZE * sizeof(size_t), i);
                recv_buffer[i][j][k] = (size_t*)cxl->GIM_malloc(SIZE * sizeof(size_t), i);
            }
        }
    }
    for (size_t i = 0; i < world_size; i++) {
        for (size_t j = 0; j < world_size; j++) {
#pragma omp parallel for
            for (int step = 0; step < SIZE; step++) {
                send_buffer[i][j][0][step] = step % 512 + 1;
                recv_buffer[i][j][0][step] = 0;
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    std::thread send_thread([&]() {
        for (int step = 1; step < world_size; step++) {   //遍历host
            int i = (world_rank - step + world_size) % world_size;
            comm.GIM_Send(reinterpret_cast<uint8_t*>(send_buffer[world_rank][i][0]), 0,SIZE*sizeof(size_t), i, 0,recv_buffer[i]);
        }
    });
    std::thread recv_thread([&]() {
        for (int step = 1; step < world_size; step++) {   //遍历host
            int i = (world_rank + step) % world_size;
            comm.GIM_Recv(0, SIZE * sizeof(size_t), i, 0);
        }
    });
    send_thread.join();
    recv_thread.join();
    MPI_Barrier(MPI_COMM_WORLD);
    size_t count = 0;
    for (int step = 1; step < world_size; step++) {   //遍历host
        int i = (world_rank + step) % world_size;
        // #pragma omp parallel for
        for (int j = 0; j < SIZE; j++) {
            if (recv_buffer[world_rank][i][0][j] != j % 512 + 1) {
                // printf("%d,%d\n",j, recv_buffer[world_rank][i][0][j]);
                count++;
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    printf("partition: %d,fault count:%ld\n", world_rank, count);
    MPI_Finalize();
}