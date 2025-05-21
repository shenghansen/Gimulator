/*
 * @Author: Hansen Sheng
 * @Date: 2024-10-13 14:15:29
 * @Last Modified by: Hansen Sheng
 * @Last Modified time: 2024-10-13 14:34:59
 */

#include "CXL_SHM.h"
#include "utills.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>

CXL_SHM::CXL_SHM(int num_hosts, int host_id, size_t cxl_shm_size, size_t gim_size) {
    this->gim_size = new size_t[num_hosts * SNC];
    this->cxl_shm_size = cxl_shm_size;
    CXL_shm = new uint8_t[cxl_shm_size];
    GIM_mem = new uint8_t*[num_hosts * SNC];
    gim_offset = new std::atomic_uint64_t[num_hosts * SNC];
    this->shm_offset = 0;
    this->num_hosts = num_hosts;
    this->host_id = host_id;
    this->shmid = new int[num_hosts * SNC + 1];
    /* shm version */
    // gim_mem init
    for (int i = 0; i < num_hosts * SNC; i++) {
        shmid[i] = shmget((key_t)i + 1, GIM_SIZE, 0666 | IPC_CREAT);
        if (shmid[i] == -1) {
            ERROR("shmat failed");
            exit(EXIT_FAILURE);
        }
        void* result = shmat(shmid[i], NULL, 0);

        if (result == (void*)-1) {
            ERROR("shmat failed");
            exit(EXIT_FAILURE);
        }
        GIM_mem[i] = static_cast<uint8_t*>(result);
        // bind to numa 0

        // if (mlock(GIM_mem[i], GIM_SIZE) != 0) {
        //     ERROR("mlock faid");
        // }
        const unsigned long mask = 1UL << i;
        if (mbind((void*)GIM_mem[i], GIM_SIZE, MPOL_BIND, &mask, 64, 0) != 0) {
            ERROR("mbind faid");
        }
        DEBUG("mbind success");
        this->gim_size[i] = GIM_SIZE;
        this->gim_offset[i] = 0;
    }
    // cxl_shm_mem init
    shmid[num_hosts * SNC] = shmget((key_t)256, cxl_shm_size, 0666 | IPC_CREAT);
    if (shmid[num_hosts * SNC] == -1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    void* result = shmat(shmid[num_hosts * SNC], NULL, 0);
    if (result == (void*)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    CXL_shm = static_cast<uint8_t*>(result);
    // bind to numa 0

    // if (mlock(CXL_shm, GIM_SIZE) != 0) {
    //     ERROR("mlock faid");
    // }
    const unsigned long mask = 1UL << 4;
    if (mbind((void*)CXL_shm, cxl_shm_size, MPOL_BIND, &mask, 64, 0) != 0) {
        ERROR("mbind faid");
    }
    // printf("mbind success\n");
    DEBUG("CXL_SHM init success");

    /* mmap version */
    // for (int i = 0; i < num_hosts * SNC; i++) {
    //     std::string base_path = "/sharenvme/usershome/shs/cxlmem/numa";
    //     std::string full_path = base_path + std::to_string(i);
    //     int shmfd = open(full_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
    //     if (shmfd == -1) {
    //         ERROR("Failed to open {}", full_path);
    //         return ;
    //     }
    //     const int mmap_port = PROT_READ | PROT_WRITE;
    //     const int mmap_flags = MAP_SHARED;
    //     uint8_t* shm_buf = (uint8_t*)mmap(NULL, GIM_SIZE, mmap_port, mmap_flags, shmfd, 0);
    //     if (shm_buf == nullptr || shm_buf == MAP_FAILED) {
    //         ERROR("Failed to mmap {}", full_path);
    //         ERROR("errno : {}", strerror(errno));
    //         exit(-1);
    //     }
    //     GIM_mem[i] = static_cast<uint8_t*>(shm_buf);
    //     if (mlock(GIM_mem[i], GIM_SIZE) != 0) {
    //         printf("mlock faid\n");
    //     }
    //     const unsigned long mask = i + 1;
    //     if (mbind((void*)GIM_mem[i], GIM_SIZE, MPOL_BIND, &mask, 64, 0) != 0) {
    //         printf("mbind failed\n");
    //     }
    //     printf("mbind success\n");
    //     this->gim_size[i] = GIM_SIZE;
    //     this->gim_offset[i] = 0;
    // }
    // // cxl_shm_mem init
    // std::string cxlshm_path = "/sharenvme/usershome/shs/cxlmem/cxlshm";
    // int shmfd = open(cxlshm_path.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
    // if (shmfd == -1) {
    //     ERROR("Failed to open {}", cxlshm_path);
    //     return;
    // }
    // const int mmap_port = PROT_READ | PROT_WRITE;
    // const int mmap_flags = MAP_SHARED;
    // uint8_t* shm_buf = (uint8_t*)mmap(NULL, GIM_SIZE, mmap_port, mmap_flags, shmfd, 0);
    // if (shm_buf == nullptr || shm_buf == MAP_FAILED) {
    //     ERROR("Failed to mmap {}", cxlshm_path);
    //     ERROR("errno : {}", strerror(errno));
    //     exit(-1);
    // }
    // CXL_shm = static_cast<uint8_t*>(shm_buf);
    // if (mlock(CXL_shm, GIM_SIZE) != 0) {
    //     printf("mlock faid\n");
    // }
    // const unsigned long mask = 1;
    // if (mbind((void*)CXL_shm, cxl_shm_size, MPOL_BIND, &mask, 64, 0) != 0) {
    //     printf("mbind failed\n");
    // }
    // DEBUG("CXL_SHM init success");
}

// CXL_SHM::~CXL_SHM() {
//     for (int i = 0; i < num_hosts * SNC; i++) {
//         // if (munlock(GIM_mem[i], GIM_SIZE) != 0) {
//         //     ERROR("mlock faid");
//         // }
//         if (shmdt(GIM_mem[i]) == -1) {
//             DEBUG("shmdt failed for segment {}", i);
//         }
//     }
//     // if (munlock(CXL_shm, GIM_SIZE) != 0) {
//     //     ERROR("mlock faid");
//     // }
//     if (shmdt(CXL_shm) == -1) {
//         DEBUG("shmdt failed for shm ");
//     }
//     if (host_id == 0) {
//         for (int i = 0; i < num_hosts + 1; i++) {
//             if (shmctl(shmid[i], IPC_RMID, 0) == -1) {
//                 DEBUG("shmctl(IPC_RMID) failed for segment {}: {}", i, strerror(errno));
//                 exit(EXIT_FAILURE);
//             }
//         }
//     }
//     delete[] GIM_mem;
//     delete gim_offset;
//     delete gim_size;
//     delete shmid;
// }

uint8_t* CXL_SHM::GIM_malloc(size_t size, int id) {
    // INFO("GIM malloc size  {} on host {} by {}", size, id*SNC,host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }
    int real_numa = id * SNC;
    uint64_t old_offset = gim_offset[real_numa].fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > gim_size[real_numa]) {
        ERROR("OOM ! GIM_mem reach the capacity limit");
        return nullptr;
    }
    return GIM_mem[real_numa] + old_offset;
}

uint8_t* CXL_SHM::GIM_malloc(size_t size, int id, int numa) {
    // INFO("GIM malloc size  {} on numa {} by {}", size, id * SNC + numa,host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }
    int real_numa = id * SNC + numa;
    uint64_t old_offset = gim_offset[real_numa].fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > gim_size[real_numa]) {
        ERROR("OOM ! GIM_mem reach the capacity limit");
        return nullptr;
    }
    return GIM_mem[real_numa] + old_offset;
}

void CXL_SHM::GIM_free(uint8_t* ptr, int id) {
    if (ptr == nullptr) {
        return;
    }
    int real_numa = id * SNC;
    if (ptr < GIM_mem[real_numa] || ptr > GIM_mem[real_numa] + gim_size[real_numa]) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}

void CXL_SHM::GIM_free(uint8_t* ptr, int id, int numa) {
    if (ptr == nullptr) {
        return;
    }
    int real_numa = id * SNC + numa;
    if (ptr < GIM_mem[real_numa] || ptr > GIM_mem[real_numa] + gim_size[real_numa]) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}

uint8_t* CXL_SHM::CXL_SHM_malloc(size_t size) {
    // INFO("cxl malloc size  {} on host {}", size, host_id);
    if (size < 64) {
        size = 64;
    } else {
        size = 64 * ((size + 63) / 64);
    }

    uint64_t old_offset = shm_offset.fetch_add(size);
    uint64_t new_offset = old_offset + size;
    if (new_offset > cxl_shm_size) {
        ERROR("OOM ! CXL_SHM reach the capacity limit");
        return nullptr;
    }
    return CXL_shm + old_offset;
}

void CXL_SHM::CXL_SHM_free(uint8_t* ptr) {
    if (ptr == nullptr) {
        return;
    }
    if (ptr < CXL_shm || ptr > CXL_shm + cxl_shm_size) {
        ERROR("free invalid pointer");
        return;
    }
    // FIXME: real mempoll not native
    return;
}