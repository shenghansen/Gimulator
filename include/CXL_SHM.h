
#include "DMA.hpp"
#include "utills.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <immintrin.h>
#include <linux/mman.h>
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

#define GIM_SIZE 64LL * 1024 * 1024 * 1024
#define CXL_SHM_SIZE  1LL * 1024 * 1024 * 1024
#define GIM_LATENCY 300
#define CACHE_LINE_SIZE 64

class CXL_SHM {
private:
    int num_hosts;
    int host_id;
    uint8_t** GIM_mem;
    uint8_t* CXL_shm;
    std::atomic_uint64_t offset;
    size_t* gim_size;
    size_t cxl_shm_size;
    int* shmid;

public:
    CXL_SHM(int num_hosts, int host_id, size_t cxl_shm_size =CXL_SHM_SIZE, size_t gim_size = GIM_SIZE);
    ~CXL_SHM();
    uint8_t* GIM_malloc(size_t size);
    void GIM_free(uint8_t* ptr);
    uint8_t* CXL_SHM_malloc(size_t size);
    void CXL_SHM_free(uint8_t* ptr);
    static void GIM_Send();
    static void GIM_Recv();
    void GIM_Read(uint8_t* source, uint8_t* destination, size_t size) {
        DMA_memcpy(source, destination, size, host_id);
    }
    void GIM_Write(uint8_t* source, uint8_t* destination, size_t size) {
        DMA_memcpy(source, destination, size, host_id);
    }

    //还是不能load结构体
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
