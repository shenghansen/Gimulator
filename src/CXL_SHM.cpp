/*
 * @Author: Hansen Sheng
 * @Date: 2024-10-13 14:15:29
 * @Last Modified by: Hansen Sheng
 * @Last Modified time: 2024-10-13 14:34:59
 */

#include "CXL_SHM.h"
#include "utills.hpp"
#include <cstddef>
#include <sys/types.h>

CXL_SHM::CXL_SHM(int num_hosts, int host_id, size_t cxl_shm_size, size_t gim_size) {
    this->gim_size = new size_t[num_hosts];
    this->cxl_shm_size = cxl_shm_size;
    CXL_shm = new uint8_t[cxl_shm_size];
    GIM_mem = new uint8_t*[num_hosts];
    gim_offset = new std::atomic_uint64_t[num_hosts];
    this->num_hosts = num_hosts;
    this->host_id = host_id;
    this->shmid = new int[num_hosts + 1];
    // gim_mem init
    for (int i = 0; i < num_hosts; i++) {
        shmid[i] = shmget((key_t)i + 1, GIM_SIZE, 0666 | IPC_CREAT);
        if (shmid[i] == -1) {
            fprintf(stderr, "shmat failed\n");
            exit(EXIT_FAILURE);
        }
        void* result = shmat(shmid[i], NULL, 0);

        if (result == (void*)-1) {
            fprintf(stderr, "shmat failed\n");
            exit(EXIT_FAILURE);
        }
        GIM_mem[i] = static_cast<uint8_t*>(result);
        // bind to numa 0

        if (mlock(GIM_mem[i], GIM_SIZE) != 0) {
            printf("mlock faid\n");
        }
        const unsigned long mask = i + 1;
        if (mbind((void*)GIM_mem[i], GIM_SIZE, MPOL_BIND, &mask, 64, 0) != 0) {
            printf("mbind failed\n");
        }
        printf("mbind success\n");
        this->gim_size[i] = GIM_SIZE;
        this->gim_offset[i] = 0;
    }
    // cxl_shm_mem init
    shmid[num_hosts] = shmget((key_t)256, cxl_shm_size, 0666 | IPC_CREAT);
    if (shmid[num_hosts] == -1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    void* result = shmat(shmid[num_hosts], NULL, 0);

    if (result == (void*)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    CXL_shm = static_cast<uint8_t*>(result);
    // bind to numa 0

    if (mlock(CXL_shm, GIM_SIZE) != 0) {
        printf("mlock faid\n");
    }
    const unsigned long mask = 1;
    if (mbind((void*)CXL_shm, cxl_shm_size, MPOL_BIND, &mask, 64, 0) != 0) {
        printf("mbind failed\n");
    }
    // printf("mbind success\n");
    DEBUG("CXL_SHM init success");
}

CXL_SHM::~CXL_SHM() {
    for (int i = 0; i < num_hosts; i++) {
        shmdt(GIM_mem[i]);
    }
    if (host_id == 0) {
        for (int i = 0; i < num_hosts + 1; i++) {
            if (shmctl(shmid[i], IPC_RMID, 0) == -1) {
                printf("shmctl(IPC_RMID) failed\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    delete[] GIM_mem;
    delete gim_offset;
    delete gim_size;
    delete shmid;
}

uint8_t* CXL_SHM::GIM_malloc(size_t size, int host_id) {
    DEBUG("cxl malloc size  {} on numa {}", size, host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }

    uint64_t old_offset = gim_offset[host_id].fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > gim_size[host_id]) {
        ERROR("OOM ! GIM_mem reach the capacity limit");
        return nullptr;
    }
    return GIM_mem[host_id] + old_offset;
}

void CXL_SHM::GIM_free(uint8_t* ptr, int id) {
    DEBUG("cxl free ptr {} size  {} on numa {}", ptr, size, id);
    if (ptr == nullptr) {
        return;
    }
    if (ptr < GIM_mem[id] || ptr > GIM_mem[id] + gim_size[id]) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}

uint8_t* CXL_SHM::CXL_SHM_malloc(size_t size) {
    DEBUG("cxl malloc size  {} on numa {}", size, host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }

    uint64_t old_offset = shm_offset.fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > cxl_shm_size) {
        ERROR("OOM ! GIM_mem reach the capacity limit");
        return nullptr;
    }
    return GIM_mem[host_id] + old_offset;
}

void CXL_SHM::CXL_SHM_free(uint8_t* ptr) {
    DEBUG("cxl free ptr {} size  {} on numa {}", ptr, size, host_id);
    if (ptr == nullptr) {
        return;
    }
    if (ptr < GIM_mem[host_id] || ptr > GIM_mem[host_id] + cxl_shm_size) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}