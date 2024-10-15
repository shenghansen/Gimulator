#include "GIM_data.h"

template<class T, bool R,int LATENCY> GIM_data<T,R,LATENCY>::GIM_data(T data){
    this->data=data;
}

template<class T, bool R,int LATENCY> GIM_data<T,R,LATENCY>& GIM_data<T,R,LATENCY>::operator=(const GIM_data<T, R,LATENCY>& a){
    if constexpr (R) {
        inject_latency_ns<LATENCY>();
        // TODO: change store size
        //  volatile char *ptr = (char *)((unsigned long)addr & ~(CACHE_LINE_SIZE - 1));
        // asm volatile("clflush %0" : "+m"(*(volatile char *)ptr));
        // asm volatile("mfence" ::: "memory");
       _mm_stream_si32(&this->data, a);
        __sync_synchronize();
       }else{
            this->data=a.data;
        return this;
    }
    
}

template<class T, bool R,int LATENCY> GIM_data<T,R,LATENCY>& GIM_data<T,R,LATENCY>::operator=(T data){
    this->data=data;
    return this;
}