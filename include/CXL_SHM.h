
#include "utills.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <immintrin.h>
#include <linux/mman.h>
#include <numa.h>
#include <numaif.h>
#include <smmintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#pragma once

#define GIM_SIZE 128uLL * 1024 * 1024 * 1024
#define CXL_SHM_SIZE 120LL * 1024 * 1024 * 1024
#define GIM_LATENCY 300
#define CACHE_LINE_SIZE 64
#define SNC 1

class CXL_SHM {
private:
    uint8_t** GIM_mem;
    uint8_t* CXL_shm;
    std::atomic_uint64_t* gim_offset;
    std::atomic_uint64_t shm_offset{0};
    size_t* gim_size;
    size_t cxl_shm_size;
    int* shmid;

public:
    int num_hosts;
    int host_id;
    CXL_SHM(int num_hosts, int host_id, size_t cxl_shm_size = CXL_SHM_SIZE,
            size_t gim_size = GIM_SIZE);
    // ~CXL_SHM();
    uint8_t* GIM_malloc(size_t size, int id);
    void GIM_free(uint8_t* ptr, int id);
    uint8_t* GIM_malloc(size_t size, int id, int numa);
    void GIM_free(uint8_t* ptr, int id, int numa);
    uint8_t* CXL_SHM_malloc(size_t size);
    void CXL_SHM_free(uint8_t* ptr);

    // 还是不能load结构体
    template<typename T> INLINE static T* load_gim(T* data, size_t index) {
        inject_latency_ns<GIM_LATENCY>();
        // TODO: change load size
        __m128i v = _mm_stream_load_si128((data + index));
        __sync_synchronize();
        return *(T*)&v;
    }

    template<typename T> INLINE static void store_gim(T* data, size_t index, T value) {
        data[index] = value;
        inject_latency_ns<GIM_LATENCY>();
        volatile char* ptr = (char*)((unsigned long)(data + index) & ~(CACHE_LINE_SIZE - 1));
        asm volatile("clflush %0" : "+m"(*(volatile char*)ptr));
        asm volatile("mfence" ::: "memory");
    }

    static INLINE void clflush(const void* data, int len) {
        volatile char* ptr = (char*)((unsigned long)data & ~(CACHE_LINE_SIZE - 1));
        for (; ptr < (char*)data + len; ptr += CACHE_LINE_SIZE) {
            asm volatile("clflush %0" : "+m"(*(volatile char*)ptr));
        }
        asm volatile("mfence" ::: "memory");
    }
};
