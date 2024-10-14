/*
 * @Author: Hansen Sheng
 * @Date: 2024-10-13 14:15:29
 * @Last Modified by: Hansen Sheng
 * @Last Modified time: 2024-10-13 14:34:59
 */

#include "CXL_SHM.h"

CXL_SHM::CXL_SHM(int num_hosts, int host_id, size_t GIM_size) {
    GIM_mem = new uint8_t*[num_hosts];
    this->num_hosts = num_hosts;
    this->host_id = host_id;
    capacity = GIM_size;
    for (int i = 0; i < num_hosts; i++) {
        int shmid;
        shmid = shmget((key_t)i + 1, GIM_SIZE, 0666 | IPC_CREAT);
        if (shmid == -1) {
            fprintf(stderr, "shmat failed\n");
            exit(EXIT_FAILURE);
        }

        // 将共享内存连接到当前进程的地址空间
        void* result = shmat(shmid, NULL, 0);

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
    }
}

CXL_SHM::~CXL_SHM() {
    for (int i = 0; i < num_hosts; i++) {
        shmdt(GIM_mem[i]);
    }
    delete[] GIM_mem;
}

uint8_t* CXL_SHM::GIM_malloc(size_t size) {
    DEBUG("cxl malloc size  {} on numa {}", size, host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }

    uint64_t old_offset = offset.fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > capacity) {
        ERROR("OOM ! GIM_mem reach the capacity limit");
        return nullptr;
    }
    return GIM_mem[host_id] + old_offset;
}

void CXL_SHM::GIM_free(uint8_t* ptr) {
    DEBUG("cxl free ptr {} size  {} on numa {}", ptr, size, host_id);
    if (ptr == nullptr) {
        return;
    }
    if (ptr < GIM_mem[host_id] || ptr > GIM_mem[host_id] + capacity) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}