/*
 * @Author: Hansen Sheng
 * @Date: 2024-10-11 10:10:42
 * @Last Modified by: Hansen Sheng
 * @Last Modified time: 2024-10-11 10:26:08
 */
#include "dml/dml.h"
#include "utills.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <omp.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std::chrono;

// TODO: Beter Strategy
#define BATCH_SIZE 8u
#define MAX_TRANSFER_SIZE 268435456u
#define MAX_DMA_SIZE BATCH_SIZE* MAX_TRANSFER_SIZE

int _batch_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
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
    status = dml_get_batch_size(dml_job_ptr, BATCH_SIZE, &batch_buffer_length);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting batch size.", static_cast<int>(status));
        return 1;
    }
    uint8_t* batch_buffer_ptr = (uint8_t*)malloc(batch_buffer_length);
    dml_job_ptr->operation = DML_OP_BATCH;
    dml_job_ptr->destination_first_ptr = batch_buffer_ptr;
    dml_job_ptr->destination_length = batch_buffer_length;
    dml_job_ptr->numa_id = numa;

    // begin batch set
    size_t buffer_size = size / BATCH_SIZE;
    size_t remainder = size % BATCH_SIZE;
    for (size_t i = 0; i < BATCH_SIZE - 1; i++) {
        status = dml_batch_set_mem_move_by_index(dml_job_ptr,
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
    status = dml_batch_set_mem_move_by_index(dml_job_ptr,
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
    status = dml_execute_job(dml_job_ptr, DML_WAIT_MODE_BUSY_POLL);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job execution.", static_cast<int>(status));
        return 1;
    }
    if (dml_job_ptr->result != 0) {
        ERROR("Operation result is incorrect.");
        return 1;
    }

    status = dml_finalize_job(dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job finalization.", static_cast<int>(status));
        free(dml_job_ptr);
        return 1;
    }
#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    INFO("Batch size: {} GB", size_in_gb);
    INFO("Batch time: {} s",
         double(duration.count()) * microseconds::period::num / microseconds::period::den);

    INFO("Batch bandwidth: {} GB/s", bandwidth);
#endif
    free(dml_job_ptr);
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    if (mlock(source, size) != 0) {
        ERROR("Failed to lock source memory!");
        return 1;
    }
    if (mlock(destination, size) != 0) {
        ERROR("Failed to lock destination memory!");
        return 1;
    }
    auto aaa = system_clock::now();
    auto ddd = duration_cast<microseconds>(aaa - start);
    INFO("mlock time: {} s",
         (double(ddd.count()) * microseconds::period::num / microseconds::period::den));
    size_t remaining_size = size;
    size_t offset = 0;
    int result;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > MAX_DMA_SIZE) ? MAX_DMA_SIZE : remaining_size;
        result = _batch_memcpy(source + offset, destination + offset, current_transfer_size, numa);
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
    // TODO: unlock?
    return 0;
}

int _batch_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa,
                               dml_job_t* dml_job_ptr) {
    // batch prepare
    dml_path_t execution_path = DML_PATH_HW;
    // dml_job_t* dml_job_ptr = NULL;
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
    status = dml_get_batch_size(dml_job_ptr, BATCH_SIZE, &batch_buffer_length);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during getting batch size.", static_cast<int>(status));
        return 1;
    }
    uint8_t* batch_buffer_ptr = (uint8_t*)malloc(batch_buffer_length);
    dml_job_ptr->operation = DML_OP_BATCH;
    dml_job_ptr->destination_first_ptr = batch_buffer_ptr;
    dml_job_ptr->destination_length = batch_buffer_length;
    dml_job_ptr->numa_id = numa;

    // begin batch set
    size_t buffer_size = size / BATCH_SIZE;
    size_t remainder = size % BATCH_SIZE;
    for (size_t i = 0; i < BATCH_SIZE - 1; i++) {
        status = dml_batch_set_mem_move_by_index(dml_job_ptr,
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
    status = dml_batch_set_mem_move_by_index(dml_job_ptr,
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
    status = dml_submit_job(dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job submit.", static_cast<int>(status));
        return 1;
    }
    return 0;
}

int _batch_memcpy_asynchronous_check(dml_job_t* dml_job_ptr) {
    dml_status_t status = dml_check_job(dml_job_ptr);
    switch (status) {
    case 0: printf("DML_STATUS_OK.\n"); break;
    case 2: printf("DML_STATUS_BEING_PROCESSED.\n"); break;
    case 23: printf("DML_STATUS_JOB_CORRUPTED.\n"); return 1;
    }
    return 0;
}

int _batch_memcpy_asynchronous_wait(dml_job_t* dml_job_ptr) {
    dml_status_t status = dml_wait_job(dml_job_ptr, DML_WAIT_MODE_UMWAIT);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job waiting.", static_cast<int>(status));
        return 1;
    }
    if (dml_job_ptr->result != 0) {
        ERROR("Operation result is incorrect.");
        return 1;
    }
    status = dml_finalize_job(dml_job_ptr);
    if (DML_STATUS_OK != status) {
        ERROR("An error {} occurred during job finalization.", static_cast<int>(status));
        free(dml_job_ptr);
        return 1;
    }
    free(dml_job_ptr);
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int DMA_memcpy_asynchronous(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    if (mlock(source, size) != 0) {
        ERROR("Failed to lock source memory!");
        return 1;
    }
    if (mlock(destination, size) != 0) {
        ERROR("Failed to lock destination memory!");
        return 1;
    }
    auto aaa = system_clock::now();
    auto ddd = duration_cast<microseconds>(aaa - start);
    INFO("mlock time: {} s",
         (double(ddd.count()) * microseconds::period::num / microseconds::period::den));
    size_t remaining_size = size;
    size_t offset = 0;
    int result;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > MAX_DMA_SIZE) ? MAX_DMA_SIZE : remaining_size;
        dml_job_t* dml_job_ptr;
        result = _batch_memcpy_asynchronous(
            source + offset, destination + offset, current_transfer_size, numa, dml_job_ptr);
        if (result != 0) {
            return 1;
        }
        result = _batch_memcpy_asynchronous_wait(dml_job_ptr);
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
    // TODO: unlock?
    return 0;
}