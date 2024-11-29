#include "CXL_SHM.h"
#include "DMA.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#pragma once
#define CAP 10

// #define SNC 2

struct GIM_status {};

struct queue_element {
    size_t size;
    int id;
    int tag;
    size_t index{0};
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
    bool pop(int index);
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
    ~GIM_comm();

    // bool GIM_Send(uint8_t* source, size_t size, int destination_id, int tag,size_t*** recv_buffer
    // );
    template<typename T>
    bool GIM_Send(uint8_t* source, size_t socket, size_t size, int destination_id, int tag,
                  T*** recv_buffer) {
        int index = send_queue[host_id]->get();
        send_queue[host_id]->data[index].size = size;
        send_queue[host_id]->data[index].id = destination_id;
        send_queue[host_id]->data[index].tag = tag;
        // send_queue[host_id]->data[index].addr=source;
        send_queue[host_id]->data[index].working_flag = 0;

        DEBUG("host {} push send ,index{}\n", host_id, index);
        // memcpy(recv_buffer[host_id][socket], source, size);
        // hl_DMA_memcpy(
        //     source, reinterpret_cast<uint8_t*>(recv_buffer[host_id][socket]), size, host_id *
        //     SNC);
        int numa_id = host_id*SNC;
        int numas = numa_num_configured_nodes();
        if (host_id >= numas) {
            numa_id = host_id % numas*SNC;
        }
        hl_DMA dma(
            source, reinterpret_cast<uint8_t*>(recv_buffer[host_id][socket]), size, numa_id);
        dma.sync();
        send_queue[host_id]->data[index].working_flag = 1;
        bool flag = true;
        while (flag) {
            if (send_queue[host_id]->data[index].working_flag == 2) {
                flag = false;
            }
        }
        send_queue[host_id]->pop(index);
        DEBUG("host {}send end \n", host_id);
        return true;
    }
    bool GIM_Recv(size_t index, size_t size, int source_id, int tag);   // index means socket
};
