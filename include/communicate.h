#include "CXL_SHM.h"
#include "DMA.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#pragma once
#define CAP 100000

// #define SNC 2

struct GIM_status {};

struct queue_element {
    size_t size;
    int id;
    int tag;
    std::atomic<int> working_flag{-1};
};

class queue {
public:
    int64_t capacity;
    std::atomic<int> index{0};
    queue_element data[CAP];   // queue_element* [capacity]
    std::mutex mtx;
    // size_t size();
    int get();
    int push(queue_element element);
    bool pop(int i);
    queue();
};


class GIM_comm {
    CXL_SHM* cxl_shm;
    queue** send_queue;   // queue* [num_hosts]
    queue** recv_queue;   // queue* [num_hosts]
    queue** completed_queue;
    int num_hosts;
    int host_id;

public:
    GIM_comm(CXL_SHM* cxl_shm);
    // ~GIM_comm();

    // bool GIM_Send(uint8_t* source, size_t size, int destination_id, int tag,size_t*** recv_buffer
    // );
    bool GIM_Send(void* source, size_t size, int destination_id, int tag, void* destination);
    bool GIM_Recv(size_t size, int source_id, int tag);   // index means socket
    bool GIM_Probe(int source_id, int tag);
    bool GIM_Get_count(int source_id, int tag, int& size);
};
