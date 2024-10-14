
// #include "include/utills.h"
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
#define GIM_SIZE 64LL * 1024 * 1024 * 1024

class CXL_SHM {
private:
    int num_hosts;
    int host_id;
    uint8_t** GIM_mem;
    std::atomic_uint64_t offset;
    size_t capacity;

public:
    CXL_SHM(int num_hosts, int host_id, size_t GIM_size = GIM_SIZE);
    ~CXL_SHM();
    uint8_t* GIM_malloc(size_t size);
    void GIM_free(uint8_t* ptr);
};
