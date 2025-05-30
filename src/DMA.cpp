#include "DMA.h"
#include "utills.hpp"

int hl_DMA::__hl_batch_memcpy_sync(uint8_t* source, uint8_t* destination, size_t size) {
    auto sequence = dml::sequence(batchsize, std::allocator<dml::byte_t>());
    dml::status_code status;

    // begin batch set
    size_t buffer_size = size / batchsize;
    size_t remainder = size % batchsize;
    // printf("buffersize is %d, remainder is %d\n",buffer_size,remainder);
    for (size_t i = 0; i < batchsize - 1; i++) {
        status = sequence.add(dml::mem_move,
                              dml::make_view(source + i * buffer_size, buffer_size),
                              dml::make_view(destination + i * buffer_size, buffer_size));
        if (status != dml::status_code::ok) {
            ERROR("An error {} occurred during setting of batch operation 1.",
                  static_cast<int>(status));
            return 1;
        }
    }

    status = sequence.add(
        dml::mem_move,
        dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
        dml::make_view(destination + (batchsize - 1) * buffer_size, buffer_size + remainder));
    if (status != dml::status_code::ok) {
        ERROR("An error {} occurred during setting of batch operation 2.",
              static_cast<int>(status));
        return 1;
    }

    // execute
    auto result = dml::execute<dml::hardware>(dml::batch, sequence, numa);
    if (result.status != dml::status_code::ok) {
        ERROR("An error {} occurred during job execution {}.",
              static_cast<int>(result.status),
              static_cast<int>(result.operations_completed));
        return 1;
    }
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}

int hl_DMA::__hl_batch_memcpy_sync_fixed(uint8_t* source, uint8_t* destination, size_t size) {
    dml::status_code status;

    // begin batch set
    size_t buffer_size = size / 4;
    if (size <= 1442296) {
        auto result = dml::execute<dml::hardware>(
            dml::mem_move, dml::make_view(source, size), dml::make_view(destination, size));
        if (result.status != dml::status_code::ok) {
            ERROR("An error {} occurred during final job execution.",
                  static_cast<int>(result.status));
            ERROR("the size is {}.", size);
            return 1;
        }
        // INFO("the size is {}.", size);
        return 0;
    }
    unsigned int transfer_size = max_transfer_size;
    if (buffer_size < max_transfer_size) {//介于两者之间，比如正好是2 max_transfer_size
        transfer_size = size / batchsize;
    }
    int bs = (size % transfer_size) ? size / transfer_size + 1 : size / transfer_size;

    auto sequence = dml::sequence(bs, std::allocator<dml::byte_t>());
    int i = 0;
    size_t remaining_size = size;
    while (remaining_size > transfer_size) {
        status = sequence.add(dml::mem_move,
                              dml::make_view(source + i * transfer_size, transfer_size),
                              dml::make_view(destination + i * transfer_size, transfer_size));
        if (status != dml::status_code::ok) {
            ERROR("An error {} occurred during setting of batch operation 1.",
                  static_cast<int>(status));
            return 1;
        }
        remaining_size -= transfer_size;
        i++;
    }

    status = sequence.add(dml::mem_move,
                          dml::make_view(source + i * transfer_size, remaining_size),
                          dml::make_view(destination + i * transfer_size, remaining_size));
    if (status != dml::status_code::ok) {
        ERROR("An error {} occurred during setting of batch operation 2.",
              static_cast<int>(status));
        return 1;
    }


    // for (size_t i = 0; i < 3; i++) {
    //     status = sequence.add(dml::mem_move,
    //                           dml::make_view(source + i * buffer_size, buffer_size),
    //                           dml::make_view(destination + i * buffer_size, buffer_size));
    //     if (status != dml::status_code::ok) {
    //         ERROR("An error {} occurred during setting of batch operation 1.",
    //               static_cast<int>(status));
    //         return 1;
    //     }
    // }

    // status = sequence.add(dml::mem_move,
    //                       dml::make_view(source + 3 * buffer_size, buffer_size + remainder),
    //                       dml::make_view(destination + 3 * buffer_size, buffer_size +
    //                       remainder));
    // if (status != dml::status_code::ok) {
    //     ERROR("An error {} occurred during setting of batch operation 2.",
    //           static_cast<int>(status));
    //     return 1;
    // }

    // execute
    auto result = dml::execute<dml::hardware>(dml::batch, sequence, numa);
    if (result.status != dml::status_code::ok) {
        ERROR("An error {} occurred during job execution {}.",
              static_cast<int>(result.status),
              static_cast<int>(result.operations_completed));
        ERROR("sequence num is {}.", i + 1);
        return 1;
    }
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}



int hl_DMA::__hl_batch_memcpy_async(uint8_t* source, uint8_t* destination, size_t size) {

    auto sequence = std::make_shared<dml::sequence<std::allocator<dml::byte_t>>>(
        batchsize, std::allocator<dml::byte_t>());
    sequences.push_back(sequence);

    dml::status_code status;

    // begin batch set
    size_t buffer_size = size / batchsize;
    size_t remainder = size % batchsize;
    for (size_t i = 0; i < batchsize - 1; i++) {
        status = sequence->add(dml::mem_move,
                               dml::make_view(source + i * buffer_size, buffer_size),
                               dml::make_view(destination + i * buffer_size, buffer_size));
        if (status != dml::status_code::ok) {
            ERROR("An error {} occurred during setting of batch operation.",
                  static_cast<int>(status));
            return 1;
        }
    }
    status = sequence->add(
        dml::mem_move,
        dml::make_view(source + (batchsize - 1) * buffer_size, buffer_size + remainder),
        dml::make_view(destination + (batchsize - 1) * buffer_size, buffer_size + remainder));
    if (status != dml::status_code::ok) {
        ERROR("An error {} occurred during setting of batch operation.", static_cast<int>(status));
        return 1;
    }
    // execute
    handlers.emplace_back(
        std::make_unique<dml::handler<dml::batch_operation, std::allocator<std::uint8_t>>>(
            dml::submit<dml::hardware>(
                dml::batch, *sequence, dml::default_execution_interface<dml::automatic>(), numa)));
    return 0;
}



int hl_DMA::sync() {
    size_t remaining_size = total_size;
    size_t offset = 0;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > max_dma_size) ? max_dma_size : remaining_size;
        // if (__hl_batch_memcpy_sync(source + offset, destination + offset, current_transfer_size))
        //     return 1;
        if (current_transfer_size == max_dma_size &&
            __hl_batch_memcpy_sync(source + offset, destination + offset, current_transfer_size))
            return 1;
        if (current_transfer_size != max_dma_size &&
            __hl_batch_memcpy_sync_fixed(
                source + offset, destination + offset, current_transfer_size))
            return 1;
        remaining_size -= current_transfer_size;
        offset += current_transfer_size;
    }
    return 0;
}
int hl_DMA::async() {
    size_t remaining_size = total_size;
    size_t offset = 0;
    while (remaining_size > 0) {
        size_t current_transfer_size =
            (remaining_size > max_dma_size) ? max_dma_size : remaining_size;
        if (__hl_batch_memcpy_async(source + offset, destination + offset, current_transfer_size))
            return 1;
        remaining_size -= current_transfer_size;
        offset += current_transfer_size;
    }
    return 0;
}
int hl_DMA::wait() {
    auto results = std::vector<dml::batch_result>();
    // synchronous operations
    for (auto& handler : handlers) {
        results.emplace_back(handler->get());
    }

    for (auto& result : results) {
        if (result.status != dml::status_code::ok) {
            ERROR("An error {} occurred during job waiting.", static_cast<int>(result.status));
            return 1;
        }
    }
    return 0;
}


int _batch_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
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
    // dml_job_ptr->flags = DML_FLAG_BLOCK_ON_FAULT;


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
    (*dml_job_ptr)->flags |= DML_FLAG_BLOCK_ON_FAULT;

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
    default: INFO("DML_STATUS_OTHER."); return 1;
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

/* By default, the library selects devices from any NUMA node within the socket of the calling
   thread. If more fine-grained control is needed, the Low-Level API of the library provides the
   ability to select devices from a specific NUMA node using the numa_id field in the job structure.
 */


int hl_DMA_memcpy_sync_test(uint8_t* source, uint8_t* destination, size_t size, int numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    auto hl_dma_sync = hl_DMA(source, destination, size, numa);
    // sync
    if (hl_dma_sync.sync()) {
        return 1;
    };
    // INFO("size is {}.",size);
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

int hl_DMA_memcpy_async_test(uint8_t* source, uint8_t* destination, size_t size, int numa) {
#ifdef STATISTICS
    auto start = system_clock::now();
#endif
    auto hl_dma_async = hl_DMA(source, destination, size, numa);
    // async
    if (hl_dma_async.async()) {
        return 0;
    };
    // sync
    int status = hl_dma_async.wait();
    if (status != 0) {
        return status;
    };

#ifdef STATISTICS
    auto end = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double size_in_gb = static_cast<double>(size) / 1024 / 1024 / 1024;
    double duration_in_seconds = static_cast<double>(duration.count()) * microseconds::period::num /
                                 microseconds::period::den;
    double bandwidth = size_in_gb / duration_in_seconds;
    INFO("DMA size: {} GB", size_in_gb);
    INFO("DMA time: {} s",
         double(duration.count()) * microseconds::period::num / microseconds::period::den);
    INFO("DMA bandwidth: {} GB/s", bandwidth);
#endif
    DEBUG("Batch Memcpy Completed Successfully.");
    return 0;
}


int hl_DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
    auto result = dml::execute<dml::hardware>(
        dml::mem_move, dml::make_view(source, size), dml::make_view(source, size), numa);

    if (result.status == dml::status_code::ok) {
        // INFO("Finished successfully.");
    } else {
        ERROR("Failure occurred.");
        return 1;
    }
    return 0;
}

int ll_DMA_memcpy(uint8_t* source, uint8_t* destination, size_t size, u_int32_t numa) {
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

    dml_job_ptr->operation = DML_OP_MEM_MOVE;
    dml_job_ptr->source_first_ptr = source;
    dml_job_ptr->destination_first_ptr = destination;
    dml_job_ptr->source_length = size;

    status = dml_execute_job(dml_job_ptr, DML_WAIT_MODE_BUSY_POLL);
    if (DML_STATUS_OK != status) {
        printf("An error (%u) occurred during job execution.\n", status);
        // printf("the job size is %d\n",size);
        dml_finalize_job(dml_job_ptr);
        free(dml_job_ptr);
        return 1;
    }
    status = dml_finalize_job(dml_job_ptr);
    if (DML_STATUS_OK != status) {
        printf("An error (%u) occurred during job finalization.\n", status);
        free(dml_job_ptr);
        return 1;
    }
    free(dml_job_ptr);

    // printf("Job Completed Successfully.\n");
    return 0;
}