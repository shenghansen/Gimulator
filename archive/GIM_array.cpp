#include "GIM_array.h"

template<typename T, bool R,int LATENCY> GIM_array<T, R,LATENCY>::GIM_array(size_t capacity, const CXL_SHM* cxl_shm) {
    this->capacity = capacity;
    this->cxl_shm = cxl_shm;
    data = (T*)this->cxl_shm->GIM_malloc(capacity * sizeof(T));
    size = 0;
}
template<typename T, bool R,int LATENCY> GIM_array<T, R,LATENCY>::~GIM_array() {
    this->cxl_shm->GIM_free(capacity);   // data will be delete in GIM_mem
}
template<typename T, bool R,int LATENCY> T& GIM_array<T, R,LATENCY>::operator[](size_t offset) {
    if constexpr (R) {
        inject_latency_ns<LATENCY>();
        // TODO: change load size
        __m128i v = _mm_stream_load_si128((data + offset));
        __sync_synchronize();
        return *(T*)&v;
    } else {
        return &data[offset];
    }
}
