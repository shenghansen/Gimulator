#include "communicate.h"
#include "DMA.h"
#include "utills.hpp"
#include <cstddef>

int queue::get() {
    std::lock_guard<std::mutex> lock(mtx);
    if (data[index % CAP].working_flag == -1) {
        data[index % CAP].working_flag = 0;
        return index++;
    }
    return  true;
}

bool queue::pop(int i) {
    std::lock_guard<std::mutex> lock(mtx);
    data[i].working_flag = -1;
    return true;
}

queue::queue() {
    // for (size_t i = 0; i <CAP; i++)
    // {
    //     data[i].working_flag=-1;
    // }
}

GIM_comm::GIM_comm(CXL_SHM* cxl_shm)
    : cxl_shm(cxl_shm) {
    this->num_hosts = cxl_shm->num_hosts;
    this->host_id = cxl_shm->host_id;
    this->send_queue = new queue*[num_hosts];
    this->recv_queue = new queue*[num_hosts];
    // this->completed_queue = new queue*[num_hosts];
    for (size_t i = 0; i < num_hosts; i++) {
        void* ptr = cxl_shm->GIM_malloc(sizeof(queue), i);
        send_queue[i] = new (ptr) queue();
        void* ptr2 = cxl_shm->GIM_malloc(sizeof(queue), i);
        recv_queue[i] = new (ptr2) queue();
        // void* ptr3 = cxl_shm->GIM_malloc(sizeof(queue), i);
        // completed_queue[i] = new (ptr) queue();
    }
}

GIM_comm::~GIM_comm() {
    for (size_t i = 0; i < num_hosts; i++) {
        cxl_shm->GIM_free(reinterpret_cast<uint8_t*>(send_queue[i]), i);
        cxl_shm->GIM_free(reinterpret_cast<uint8_t*>(recv_queue[i]), i);
    }
    delete[] send_queue;
    delete[] recv_queue;
}

// bool GIM_comm::GIM_Send(uint8_t* source, size_t size, int destination_id, int tag,
//                         size_t*** recv_buffer) {
//     // queue_element element ;
//     // element.size = size;
//     // element.id = destination_id;
//     // element.tag = tag;
//     // element.addr = source;
//     // element.working_flag = 0;
//     // send_queue[host_id]->push(element);

//     int index = send_queue[host_id]->get();
//     send_queue[host_id]->data[index].size = size;
//     send_queue[host_id]->data[index].id = destination_id;
//     send_queue[host_id]->data[index].tag = tag;
//     // send_queue[host_id]->data[index].addr=source;
//     send_queue[host_id]->data[index].working_flag = 0;

//     DEBUG("push send ,index{}\n",index);
//     bool flag = true;
//     while (flag) {
//         if (send_queue[host_id]->data[index].working_flag == 1) {
//             // DEBUG("begin send\n");
//             for (auto& q : recv_queue[destination_id]->data) {
//                 if (q.working_flag == 0 && q.id == host_id && q.size == size) {
//                     // hl_DMA dma(source, recv_buffer[host_id][q.index], size, host_id * SNC);
//                     // dma.sync();
//                     memcpy(recv_buffer[host_id][q.index], source, size);
//                     q.working_flag = 1;
//                     flag = false;
//                     break;
//                 }
//             }
//         }
//     }
//     send_queue[host_id]->pop(index);
//     DEBUG("send end \n");
//     return true;
// }
// template<typename T>
// bool GIM_comm::GIM_Send(uint8_t* source, size_t socket, size_t size, int destination_id, int tag,
//                         T*** recv_buffer) {
//     // queue_element element ;
//     // element.size = size;
//     // element.id = destination_id;
//     // element.tag = tag;
//     // element.addr = source;
//     // element.working_flag = 0;
//     // send_queue[host_id]->push(element);

//     int index = send_queue[host_id]->get();
//     send_queue[host_id]->data[index].size = size;
//     send_queue[host_id]->data[index].id = destination_id;
//     send_queue[host_id]->data[index].tag = tag;
//     // send_queue[host_id]->data[index].addr=source;
//     send_queue[host_id]->data[index].working_flag = 0;

//     DEBUG("host {} push send ,index{}\n",host_id, index);
//     // memcpy(recv_buffer[host_id][socket], source, size);
//     hl_DMA_memcpy(source, reinterpret_cast<uint8_t*>(recv_buffer[host_id][socket]), size, host_id * SNC);
//     // hl_DMA dma(source, recv_buffer[host_id][socket], size, host_id * SNC);
//     // dma.sync();
//     send_queue[host_id]->data[index].working_flag = 1;
//     bool flag = true;
//     while (flag) {
//         if (send_queue[host_id]->data[index].working_flag == 2) {
//             flag = false;
//         }
//     }
//     send_queue[host_id]->pop(index);
//     DEBUG("host {}send end \n",host_id);
//     return true;
// }


bool GIM_comm::GIM_Recv(size_t size, int source_id, int tag) {

    bool flag = true;
    while (flag) {
        for (auto& q : send_queue[source_id]->data) {
            if (q.working_flag != -1 && q.id == host_id && q.size == size) {
                DEBUG("recv {} find send",host_id);
                while (flag) {
                    if (q.working_flag == 1) {
                        q.working_flag = 2;
                        flag = false;
                    }
                }
                break;
            }
        }
    }

    // bool GIM_comm::GIM_Recv(size_t index, size_t size, int source_id, int tag) {
    //     // queue_element* element = new queue_element();
    //     // element->size = size;
    //     // element->id = source_id;
    //     // element->tag = tag;
    //     // element->index = index;
    //     // element->working_flag = 0;
    //     // recv_queue[host_id]->push(element);

    //     int i = recv_queue[host_id]->get();
    //     recv_queue[host_id]->data[i].size = size;
    //     recv_queue[host_id]->data[i].id = source_id;
    //     recv_queue[host_id]->data[i].tag = tag;
    //     send_queue[host_id]->data[i].index = index;
    //     recv_queue[host_id]->data[i].working_flag = 0;
    //     DEBUG("push recv \n");

    //     bool flag = true;
    //     while (flag) {
    //         for (auto& q : send_queue[source_id]->data) {
    //             if (q.working_flag == 0 && q.id == host_id && q.size == size) {
    //                 DEBUG("begin recv\n");
    //                 q.working_flag = 1;
    //                 while (flag) {
    //                     if (recv_queue[host_id]->data[i].working_flag == 1) {
    //                         flag = false;
    //                     }
    //                 }
    //                 break;
    //             }
    //         }
    //     }
    //     recv_queue[host_id]->pop(i);
    //     DEBUG("recv end\n");
    //     return true;
    // }

    DEBUG("recv end\n");
    return true;
}