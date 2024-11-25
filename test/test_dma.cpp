#include "CXL_SHM.h"
#include "DMA.h"
#include <dml/hl/execution_path.hpp>
#include <iostream>
#include <mpi.h>
int main(int argc, char** argv) {
    size_t size = std::stoull(argv[1]);
    uint8_t* source = new uint8_t[size];
    uint8_t* destination = new uint8_t[size];

    /* The mlock function locks a specified region of memory into physical memory,
    preventing these memory pages from being swapped out to the swap space. */
    if (mlock(source, size) != 0) {
        std::cerr << "Failed to lock source memory!" << std::endl;
        free(source);
        return -1;
    }
    if (mlock(destination, size) != 0) {
        std::cerr << "Failed to lock destination memory!" << std::endl;
        free(destination);
        return -1;
    }

//     MPI_Init(&argc, &argv);   // 初始化 MPI 环境
//     int world_size;
//     MPI_Comm_size(MPI_COMM_WORLD, &world_size);   // 获取进程总数
//     int world_rank;
//     MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);   // 获取当前进程编号
//     printf("word_size %d\n", world_size);

//     CXL_SHM cxl_shm(world_size, world_rank);
//     uint8_t* source = cxl_shm.GIM_malloc(size, 0);
//     uint8_t* destination = cxl_shm.GIM_malloc(size, 1);

//     if (world_rank == 0) {
// #pragma omp parallel for
//         for (size_t i = 0; i < size; i++) {
//             source[i] = i % 256;
//         }
//         ll_DMA_memcpy(source, destination, size, 0);
//     }



    DMA_memcpy(source, destination, size,  0);
    // DMA_memcpy_asynchronous(source, destination, size, 1);
    // hl_DMA_memcpy_sync_test(source, destination, size, 0);
    // hl_DMA_memcpy_async_test(source, destination, size, 0);
}