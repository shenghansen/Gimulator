
#include "utills.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <linux/mman.h>
#include <numaif.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include "DMA.hpp"


#define GIM_SIZE 64LL * 1024 * 1024 * 1024
#define CXL_SHM_SIZE  1LL * 1024 * 1024 * 1024
#define GIM_LATENCY 300
template<typename T> INLINE T* load_gim(T* data,size_t index){
    inject_latency_ns<GIM_LATENCY>();
    // TODO: change load size
    __m128i v = _mm_stream_load_si128((data + index));
    __sync_synchronize();
    return *(T*)&v;
}
template<typename T> INLINE void store_gim(T* data,size_t index,T value){
     inject_latency_ns<GIM_LATENCY>();
    // TODO: change load size
    __m128i v = _mm_stream_si32((data+index), value);
    __sync_synchronize();
}

class CXL_SHM {
private:
    int num_hosts;
    int host_id;
    uint8_t** GIM_mem;
    uint8_t* CXL_shm;
    std::atomic_uint64_t offset;
    size_t* gim_size;
    size_t cxl_shm_size;



public:
    CXL_SHM(int num_hosts, int host_id, size_t cxl_shm_size =CXL_SHM_SIZE, size_t gim_size = GIM_SIZE);
    ~CXL_SHM();
    uint8_t* GIM_malloc(size_t size);
    void GIM_free(uint8_t* ptr);
    uint8_t* CXL_SHM_malloc(size_t size);
    void CXL_SHM_free(uint8_t* ptr);
    static void GIM_Send();
    static void GIM_Recv();


};
