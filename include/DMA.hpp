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
#include <omp.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <memory>
using namespace std::chrono;

// TODO: Beter Strategy
//  DML_MIN_BATCH_SIZE is 4u
#define BATCH_SIZE 8u
#define MAX_TRANSFER_SIZE 268435456u
#define MAX_DMA_SIZE BATCH_SIZE* MAX_TRANSFER_SIZE

class hl_DMA_async{
    private:
        uint8_t* source;
        uint8_t* destination;
        size_t total_size;
        unsigned int batchsize;
        unsigned int max_transfer_size;
        uint64_t max_dma_size;
        // std::vector<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>> handlers;
        std::vector<std::shared_ptr<dml::sequence<std::allocator<dml::byte_t>>>> sequences;
        std::vector<std::unique_ptr<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>>> handlers;
        int __hl_batch_memcpy_async(uint8_t* source, uint8_t* destination, size_t size){

            // auto sequence = dml::sequence(batchsize, std::allocator<dml::byte_t>());
            auto sequence = std::make_shared<dml::sequence<std::allocator<dml::byte_t>> >(
                batchsize, std::allocator<dml::byte_t>());
            sequences.push_back(sequence);

            dml::status_code status;

            // begin batch set
            size_t buffer_size = size / batchsize;
            size_t remainder = size % batchsize;
            for(size_t i=0; i < batchsize - 1; i++){
                status = sequence->add(dml::mem_move, 
                                    dml::make_view(source + i * buffer_size, buffer_size),
                                    dml::make_view(destination + i * buffer_size, buffer_size));
                if(status != dml::status_code::ok){
                    ERROR("An error {} occurred during setting of batch operation.",
                        static_cast<int>(status));
                    return 1;
                }
            }
            status = sequence->add(dml::mem_move,
                                dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
                                dml::make_view(destination + (batchsize - 1) * buffer_size, buffer_size + remainder));
            if(status != dml::status_code::ok){
                ERROR("An error {} occurred during setting of batch operation.",
                        static_cast<int>(status));
                return 1;
            }
            // execute
            // handlers.emplace_back(dml::submit<dml::hardware>(dml::batch, sequence));
            handlers.emplace_back(std::make_unique<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>>(
                dml::submit<dml::hardware>(dml::batch, *sequence)));
            return 0;
        }
//         int __hl_batch_memcpy_async(uint8_t* source, uint8_t* destination, size_t size){
//             dml::sequence<>* sequence =
//                 new dml::sequence(batchsize, std::allocator<std::uint8_t>());
//             // auto sequence = dml::sequence(batchsize, std::allocator<std::uint8_t>());
//             dml::status_code status;

//             // begin batch set
//             size_t buffer_size = size / batchsize;
//             size_t remainder = size % batchsize;
//             for(size_t i=0; i < batchsize - 1; i++){
//                 status = sequence->add(dml::mem_move,
//                                        dml::make_view(source + i * buffer_size, buffer_size),
//                                        dml::make_view(destination + i * buffer_size, buffer_size));
//                 if(status != dml::status_code::ok){
//                     ERROR("An error {} occurred during setting of batch operation.",
//                         static_cast<int>(status));
//                     return 1;
//                 }
//             }
//             status = sequence->add(
//                 dml::mem_move,
//                 dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
//                 dml::make_view(destination + (batchsize - 1) * buffer_size,
//                                buffer_size + remainder));
//             if(status != dml::status_code::ok){
//                 ERROR("An error {} occurred during setting of batch operation.",
//                         static_cast<int>(status));
//             return 1;
//             }
//             // execute
//             dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>* handler =
//                 new dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>();
//             *handler = dml::submit<dml::hardware>(dml::batch, *sequence);
//             handlers.push_back(handler);
//             return 0;
//             }
    public:
        hl_DMA_async(uint8_t* source, uint8_t* destination, size_t total_size,
                     unsigned int batchsize, unsigned int max_transfer_size):source(source),
                                                                             destination(destination),
                                                                             total_size(total_size),
                                                                             batchsize(batchsize), 
                                                                             max_transfer_size(max_transfer_size){
            max_dma_size = batchsize * max_transfer_size;
            handlers.reserve(batchsize);
        };
        // ~hl_DMA_async(){
            // delete[] source;
            // delete[] destination;
            // handlers.clear();
            // handlers.shrink_to_fit();
        // };
        int async(){
            size_t remaining_size = total_size;
            size_t offset = 0;
            while (remaining_size > 0) {
                size_t current_transfer_size =
                    (remaining_size > max_dma_size) ? max_dma_size : remaining_size;
                        if(__hl_batch_memcpy_async(source + offset,
                                                destination + offset,
                                                current_transfer_size))return 1;
                remaining_size -= current_transfer_size;
                offset += current_transfer_size;
            }
            return 0;
        }
        int wait(){
            auto results = std::vector<dml::batch_result>();
            // synchronous operations
            for (auto& handler : handlers) {
                // auto result = handler.get();
                // if (result.status != dml::status_code::ok) {
                //     ERROR("An error {} occurred during job waiting.", static_cast<int>(result.status));
                //     return 1;
                // }
                results.emplace_back(handler->get());
            }

            for(auto& result : results){
                if (result.status != dml::status_code::ok) {
                    // ERROR("An error {} occurred during job waiting.", static_cast<int>(result.status));
                    return 1;
                }
            }
            return 0;
        }
};

int _batch_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa, unsigned int batchsize) {
// #ifdef STATISTICS
//     auto start = system_clock::now();
// #endif
    // std::vector<uint8_t> source_v(source, source + size);
    // std::vector<uint8_t> destination_v(destination, destination + size);
    // dml::execute<dml::hardware>(
    //     dml::mem_move, dml::make_view(source_v), dml::make_view(destination_v), numa);

    // batch prepare
    dml_path_t execution_path = DML_PATH_HW;
    dml_job_t* dml_job_ptr = NULL;
    uint32_t job_size = 0u;
    dml_status_t status = dml_get_job_size(execution_path, &job_size);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting job size.", static_cast<int>(status));
        return 1;
    }
    dml_job_ptr = (dml_job_t*)malloc(job_size);
    status = dml_init_job(execution_path, dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job initialization.", static_cast<int>(status));
        free(dml_job_ptr);
        return 1;
    }
    uint32_t batch_buffer_length = 0u;
    status = dml_get_batch_size(dml_job_ptr, batchsize, &batch_buffer_length);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting batch size.", static_cast<int>(status));
        dml_finalize_job(dml_job_ptr);
        free(dml_job_ptr);
        return 1;
    }
    uint8_t* batch_buffer_ptr = (uint8_t*)malloc(batch_buffer_length);
    dml_job_ptr->operation = DML_OP_BATCH;
    dml_job_ptr->destination_first_ptr = batch_buffer_ptr;
    dml_job_ptr->destination_length = batch_buffer_length;
    dml_job_ptr->numa_id = numa;

    // begin batch set
    size_t buffer_size = size / batchsize;
    size_t remainder = size % batchsize;
    for (size_t i = 0; i < batchsize - 1; i++) {
        status = dml_batch_set_mem_move_by_index(dml_job_ptr,
                                                 i,
                                                 source + i * buffer_size,
                                                 destination + i * buffer_size,
                                                 buffer_size,
                                                 DML_FLAG_PREFETCH_CACHE);
        if (DML_STATUS_OK != status) {
            ERROR("An error {} occurred during setting of batch operation.",
                  static_cast<int>(status));
            dml_finalize_job(dml_job_ptr);
            free(dml_job_ptr);
            return 1;
        }
    }
    status = dml_batch_set_mem_move_by_index(dml_job_ptr,
                                             batchsize - 1,
                                             source + (batchsize - 1) * buffer_size,
                                             destination + (batchsize - 1) * buffer_size,
                                             buffer_size + remainder,
                                             DML_FLAG_PREFETCH_CACHE);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during setting of batch operation.", static_cast<int>(status));
        dml_finalize_job(dml_job_ptr);
        free(dml_job_ptr);
        return 1;
    }
    // execute
    status = dml_execute_job(dml_job_ptr, DML_WAIT_MODE_BUSY_POLL);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job execution.", static_cast<int>(status));
        dml_finalize_job(dml_job_ptr);
        free(dml_job_ptr);
        return 1;
    }
    if (dml_job_ptr->result != 0) {
        ERROR("Operation result is incorrect.");
        dml_finalize_job(dml_job_ptr);
        free(dml_job_ptr);
        return 1;
    }

    status = dml_finalize_job(dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job finalization.", static_cast<int>(status));
        free(dml_job_ptr);
        return 1;
    }
// #ifdef STATISTICS
//     auto end = system_clock::now();
//     auto duration = duration_cast<microseconds>(end - start);
//     double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
//     double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
//                                  microseconds::period::den;
//     double bandwidth = size_in_gb / duration_in_seconds;
//     INFO("Batch size: {} GB", size_in_gb);
//     INFO("Batch time: {} s",
//          double(duration.count()) * microseconds::period::num / microseconds::period::den);

//     INFO("Batch bandwidth: {} GB/s", bandwidth);
// #endif
    free(dml_job_ptr);
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa, unsigned int batchsize, unsigned int max_transfer_size) {
    unsigned long max_dma_size = batchsize * max_transfer_size;
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    size_t remaining_size = size;
    size_t offset = 0;
    int result;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > max_dma_size ) ? max_dma_size  : remaining_size;
        result = _batch_memcpy(source + offset, destination + offset, current_transfer_size, numa, batchsize);
        if (result != 0) {
            return 1;
        }
        remaining_size -= current_transfer_size;
        offset += current_transfer_size;
    }
#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    INFO("DMA size: {} GB", size_in_gb);
    INFO("DMA time: {} s",
         (double(duration.count()) * microseconds::period::num / microseconds::period::den));

    INFO("DMA bandwidth: {} GB/s", bandwidth);
#endif
    return 0;
}

int _batch_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa,
                               dml_job_t** dml_job_ptr) {

    // batch prepare
    dml_path_t execution_path = DML_PATH_HW;
    // dml_job_t* dml_job_ptr = NULL;
    uint32_t job_size = 0u;
    dml_status_t status = dml_get_job_size(execution_path, &job_size);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting job size.", static_cast<int>(status));
        return 1;
    }
    *dml_job_ptr = (dml_job_t*)malloc(job_size);
    status = dml_init_job(execution_path, *dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job initialization.", static_cast<int>(status));
        free(*dml_job_ptr);
        return 1;
    }
    uint32_t batch_buffer_length = 0u;
    status = dml_get_batch_size(*dml_job_ptr, BATCH_SIZE, &batch_buffer_length);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting batch size.", static_cast<int>(status));
        return 1;
    }
    uint8_t* batch_buffer_ptr = (uint8_t*)malloc(batch_buffer_length);
    (*dml_job_ptr)->operation = DML_OP_BATCH;
    (*dml_job_ptr)->destination_first_ptr = batch_buffer_ptr;
    (*dml_job_ptr)->destination_length = batch_buffer_length;
    (*dml_job_ptr)->numa_id = numa;

    // begin batch set
    size_t buffer_size = size / BATCH_SIZE;
    size_t remainder = size % BATCH_SIZE;
    for (size_t i = 0; i < BATCH_SIZE - 1; i++) {
        status = dml_batch_set_mem_move_by_index(*dml_job_ptr,
                                                 i,
                                                 source + i * buffer_size,
                                                 destination + i * buffer_size,
                                                 buffer_size,
                                                 DML_FLAG_PREFETCH_CACHE);
        if (DML_STATUS_OK != status) {
            ERROR("An error {} occurred during setting of batch operation.",
                  static_cast<int>(status));
            return 1;
        }
    }
    status = dml_batch_set_mem_move_by_index(*dml_job_ptr,
                                             BATCH_SIZE - 1,
                                             source + (BATCH_SIZE - 1) * buffer_size,
                                             destination + (BATCH_SIZE - 1) * buffer_size,
                                             buffer_size + remainder,
                                             DML_FLAG_PREFETCH_CACHE);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during setting of batch operation.", static_cast<int>(status));
        return 1;
    }
    // execute
    status = dml_submit_job(*dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job submit.", static_cast<int>(status));
        return 1;
    }
    return 0;
}

int _batch_memcpy_asynchronous_check(dml_job_t** dml_job_ptr) {
    dml_status_t status = dml_check_job(*dml_job_ptr);
    switch (status) {
    case 0: INFO("DML_STATUS_OK."); break;
    case 2: INFO("DML_STATUS_BEING_PROCESSED."); break;
    case 23: INFO("DML_STATUS_JOB_CORRUPTED."); return 1;
    }
    return 0;
}

int _batch_memcpy_asynchronous_wait(dml_job_t** dml_job_ptr) {
    dml_status_t status = dml_wait_job(*dml_job_ptr, DML_WAIT_MODE_UMWAIT);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job waiting.", static_cast<int>(status));
        return 1;
    }
    if ((*dml_job_ptr)->result != 0) {
        ERROR("Operation result is incorrect.");
        return 1;
    }
    status = dml_finalize_job(*dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job finalization.", static_cast<int>(status));
        free(*dml_job_ptr);
        return 1;
    }
    free(*dml_job_ptr);
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int DMA_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    size_t remaining_size = size;
    size_t offset = 0;
    int result;
    std::vector<dml_job_t*> dml_job_ptrs;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > MAX_DMA_SIZE) ? MAX_DMA_SIZE : remaining_size;
        dml_job_t* dml_job_ptr;
        result = _batch_memcpy_asynchronous(
            source + offset, destination + offset, current_transfer_size, numa, &dml_job_ptr);
        if (result != 0) {
            return 1;
        }
        dml_job_ptrs.push_back(dml_job_ptr);
        remaining_size -= current_transfer_size;
        offset += current_transfer_size;
    }
    for (auto it : dml_job_ptrs) {
        result = _batch_memcpy_asynchronous_wait(&it);
        if (result != 0) {
            return 1;
        }
    }

#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    INFO("DMA size: {} GB", size_in_gb);
    INFO("DMA time: {} s",
         (double(duration.count()) * microseconds::period::num / microseconds::period::den));

    INFO("DMA bandwidth: {} GB/s", bandwidth);
#endif
    return 0;
}

/* By default, the library selects devices from any NUMA node within the socket of the calling thread. 
   If more fine-grained control is needed, the Low-Level API of the library provides the ability to 
   select devices from a specific NUMA node using the numa_id field in the job structure. */
int _hl_batch_memcpy(uint8_t* source, uint8_t* destination, size_t size, unsigned int batchsize){
// #ifdef STATISTICS
//     auto start = system_clock::now();
// #endif
    auto sequence = dml::sequence(batchsize, std::allocator<dml::byte_t>());
    dml::status_code status;

    // begin batch set
    size_t buffer_size = size / batchsize;
    size_t remainder = size % batchsize;
    for(size_t i=0; i < batchsize - 1; i++){
        status = sequence.add(dml::mem_move, 
                              dml::make_view(source + i * buffer_size, buffer_size),
                              dml::make_view(destination + i * buffer_size, buffer_size));
        if(status != dml::status_code::ok){
            ERROR("An error {} occurred during setting of batch operation 1.",
                  static_cast<int>(status));
            return 1;
        }
    }

    status = sequence.add(dml::mem_move,
                    dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
                    dml::make_view(destination + (batchsize - 1) * buffer_size, buffer_size + remainder));
    if(status != dml::status_code::ok){
        ERROR("An error {} occurred during setting of batch operation 2.",
                static_cast<int>(status));
        return 1;
    }

    // execute
    auto result = dml::execute<dml::hardware>(dml::batch, sequence);
    if(result.status != dml::status_code::ok){
        ERROR("An error {} occurred during job execution {}.", static_cast<int>(result.status), static_cast<int>(result.operations_completed));
        return 1;
    }
// #ifdef STATISTICS
//     auto end = system_clock::now();
//     auto duration = duration_cast<microseconds>(end - start);
//     double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
//     double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
//                                  microseconds::period::den;
//     double bandwidth = size_in_gb / duration_in_seconds;
//     INFO("Batch size: {} GB", size_in_gb);
//     INFO("Batch time: {} s",
//          double(duration.count()) * microseconds::period::num / microseconds::period::den);

//     INFO("Batch bandwidth: {} GB/s", bandwidth);
// #endif
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int hl_DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, unsigned int batchsize, unsigned int max_transfer_size){
    // INFO("Total size: {}, Batchsize: {}, MAX_TRANSFER_SIZE:{}", size, batchsize, max_transfer_size);
    unsigned long max_dma_size = batchsize * max_transfer_size;
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    size_t remaining_size = size;
    size_t offset = 0;
    int result;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > max_dma_size) ? max_dma_size : remaining_size;
        result = _hl_batch_memcpy(source + offset, destination + offset, current_transfer_size, batchsize);
        if (result != 0) {
            return 1;
        }
        remaining_size -= current_transfer_size;
        offset += current_transfer_size;
    }
#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    INFO("DMA size: {} GB", size_in_gb);
    INFO("DMA time: {} s",
         (double(duration.count()) * microseconds::period::num / microseconds::period::den));

    INFO("DMA bandwidth: {} GB/s", bandwidth);
#endif
    return 0;
}

// dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>
//  _hl_batch_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, unsigned int batchsize){
//     auto sequence = dml::sequence(batchsize, std::allocator<dml::byte_t>());
//     dml::status_code status;

//     // begin batch set
//     size_t buffer_size = size / batchsize;
//     size_t remainder = size % batchsize;
//     for(size_t i=0; i < batchsize - 1; i++){
//         status = sequence.add(dml::mem_move, 
//                               dml::make_view(source + i * buffer_size, buffer_size),
//                               dml::make_view(destination + i * buffer_size, buffer_size));
//         if(status != dml::status_code::ok){
//             ERROR("An error {} occurred during setting of batch operation.",
//                   static_cast<int>(status));
//             // return 1;
//             exit(0);
//         }
//     }
//     status = sequence.add(dml::mem_move,
//                           dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
//                           dml::make_view(destination + (batchsize - 1) * buffer_size, buffer_size + remainder));
//     if(status != dml::status_code::ok){
//         ERROR("An error {} occurred during setting of batch operation.",
//                 static_cast<int>(status));
//         // return 1;
//         exit(0);
//     }
//     // execute
//     auto handler = dml::submit<dml::hardware>(dml::batch, sequence);
//     return std::move(handler);    
// }

// std::vector<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>> 
// hl_DMA_memcpy_sync(uint8_t* source, uint8_t* destination, size_t size, unsigned int batchsize, unsigned int max_transfer_size) {
//     unsigned long max_dma_size = batchsize * max_transfer_size;
//     size_t remaining_size = size;
//     size_t offset = 0;
//     std::vector<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>> handlers;

//     while (remaining_size > 0) {
//         size_t current_transfer_size =
//             (remaining_size > max_dma_size) ? max_dma_size : remaining_size;
//         handlers.emplace_back(_hl_batch_memcpy_asynchronous(source + offset,
//                                                             destination + offset,
//                                                             current_transfer_size,
//                                                             batchsize));
//         remaining_size -= current_transfer_size;
//         offset += current_transfer_size;
//     }

//     // Copy Elision ?
//     // return std::move(handlers);
//     return handlers;
// }

// int hl_DMA_memcpy_wait(std::vector<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>>& handlers) {
//     // synchronous operations
//     for (auto& handler : handlers) {
//         auto result = handler.get();
//         if (result.status != dml::status_code::ok) {
//             ERROR("An error {} occurred during job waiting.", static_cast<int>(result.status));
//             return 1;
//         }
//     }
//     return 0;
// }

int hl_DMA_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, unsigned int batchsize, unsigned int max_transfer_size) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    // auto handlers = hl_DMA_memcpy_sync(source, destination, size, batchsize, max_transfer_size);

    // int status = hl_DMA_memcpy_wait(handlers);
    // if (status != 0) {
    //     return status;
    // }
    auto hl_dma_async = hl_DMA_async(source, destination, size, batchsize, max_transfer_size);
    // async
    if(hl_dma_async.async()){
        return 0;
    };
    // INFO("batch job are all launched");s
    // sync
    int status = hl_dma_async.wait();
    if(status != 0){
        return status;
    };

#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    // spdlog::set_default_logger(spdlog::stdout_color_mt("console"));
    INFO("DMA size: {} GB", size_in_gb);
    INFO("DMA time: {} s", double(duration.count()) * microseconds::period::num / microseconds::period::den);
    INFO("DMA bandwidth: {} GB/s", bandwidth);
    // printf("DMA size: %f GB", size_in_gb);
    // printf("DMA time: %f s", double(duration.count()) * microseconds::period::num / microseconds::period::den);
    // printf("DMA bandwidth: %f GB/s", bandwidth);

#endif
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}
