#include "communicate.h"
#include "mpi.h"
#include "utills.hpp"
#include <cstddef>
#include <cstdint>
#include <omp.h>
#include <thread>
#define SIZE 2048
#define NUMA 4
#define PARTITIONS 2


int main(int argc, char** argv) {
    // spdlog::set_level(spdlog::level::debug);

    /* multi process test */
    MPI_Init(&argc, &argv);   // 初始化 MPI 环境
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);   // 获取进程总数
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);   // 获取当前进程编号
    printf("word_size %d\n", world_size);

    CXL_SHM* cxl = new CXL_SHM(world_size, world_rank);
    GIM_comm comm(cxl);

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

        for (size_t j = 0; j < world_size; j++) {
#pragma omp parallel for
            for (size_t k = 0; k < NUMA; k++) {
                for (int step = 0; step < SIZE; step++) {
                    send_buffer[world_rank][j][k][step] = step % 512 + 1;
                    recv_buffer[world_rank][j][k][step] = 0;
                }
            }
        }

    MPI_Barrier(MPI_COMM_WORLD);
    INFO("aaa");
    std::thread send_thread([&]() {
        for (int step = 1; step < world_size; step++) {   //遍历host
            int i = (world_rank - step + world_size) % world_size;
            for (size_t s_i = 0; s_i < NUMA; s_i++) {
                comm.GIM_Send(send_buffer[world_rank][i][s_i],
                              SIZE * sizeof(size_t),
                              i,
                              0,
                              recv_buffer[i][world_rank][s_i]);
            }
        }
    });
    std::thread recv_thread([&]() {
        for (int step = 1; step < world_size; step++) {   //遍历host
            int i = (world_rank + step) % world_size;
            comm.GIM_Probe(i,0);
            size_t comm_size=0;
            comm.GIM_Get_count(i,0,comm_size);
            for (size_t s_i = 0; s_i < NUMA; s_i++) {
                comm.GIM_Recv(comm_size, i, 0);
            }
        }
    });
    INFO("aaa");
    send_thread.join();
    recv_thread.join();
    MPI_Barrier(MPI_COMM_WORLD);
    INFO("aaa");
    size_t count = 0;
    for (int step = 1; step < world_size; step++) {   //遍历host
        int i = (world_rank + step) % world_size;
        // #pragma omp parallel for
        for (size_t s_i = 0; s_i < NUMA; s_i++) {
            for (int j = 0; j < SIZE; j++) {
                if (recv_buffer[world_rank][i][s_i][j] != j % 512 + 1) {
                    // printf("%d,%d\n",j, recv_buffer[world_rank][i][0][j]);
                    count++;
                }
            }
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    printf("partition: %d,fault count:%ld\n", world_rank, count);
    MPI_Finalize();
}