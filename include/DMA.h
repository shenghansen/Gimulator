/*
 * @Author: Hansen Sheng
 * @Date: 2024-10-11 10:10:42
 * @Last Modified by: Hansen Sheng
 * @Last Modified time: 2024-10-11 16:08:14
 */

#include "utills.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dml/dml.h>
#include <dml/dml.hpp>
#include <memory>
#include <omp.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
using namespace std::chrono;
#pragma once

// TODO: Beter Strategy
//  DML_MIN_BATCH_SIZE is 4u
#define BATCH_SIZE 8u
// #define BATCH_SIZE 1u
#define MAX_TRANSFER_SIZE 268435456u
// #define MAX_TRANSFER_SIZE 4096u
#define MAX_DMA_SIZE BATCH_SIZE* MAX_TRANSFER_SIZE

class hl_DMA {
private:
    int numa;
    uint8_t* source;
    uint8_t* destination;
    size_t total_size;
    unsigned int batchsize;
    unsigned int max_transfer_size;
    uint64_t max_dma_size;
    std::vector<std::shared_ptr<dml::sequence<std::allocator<dml::byte_t>>>> sequences;
    std::vector<std::unique_ptr<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>>>
        handlers;

    int __hl_batch_memcpy_sync(uint8_t* source, uint8_t* destination, size_t size);
    int __hl_batch_memcpy_async(uint8_t* source, uint8_t* destination, size_t size);

public:
    hl_DMA(uint8_t* source, uint8_t* destination, size_t total_size, int numa)
        : source(source)
        , destination(destination)
        , total_size(total_size)
        , numa(numa){
            batchsize=BATCH_SIZE;
            max_transfer_size=MAX_TRANSFER_SIZE;
            max_dma_size = MAX_DMA_SIZE;};
    int sync();
    int async();
    int wait();
};

int _batch_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa);
int DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa);

int _batch_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa,
                               dml_job_t** dml_job_ptr);

int _batch_memcpy_asynchronous_check(dml_job_t** dml_job_ptr);

int _batch_memcpy_asynchronous_wait(dml_job_t** dml_job_ptr);

int DMA_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa);

/* By default, the library selects devices from any NUMA node within the socket of the calling
   thread. If more fine-grained control is needed, the Low-Level API of the library provides the
   ability to select devices from a specific NUMA node using the numa_id field in the job structure.
 */


int hl_DMA_memcpy_sync_test(uint8_t* source, uint8_t* destination, size_t size,
                            int numa);
int hl_DMA_memcpy_async_test(uint8_t* source, uint8_t* destination, size_t size,
                             int numa);

int hl_DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa);

int ll_DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa);