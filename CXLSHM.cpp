
#include "utills.h"
#include <fcntl.h>
#include <linux/mman.h>
#include <numaif.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>

#define GIM_SIZE 64 * 1024 * 1024 * 1024

class CXL_SHM {
private:
    int num_hosts;
    int host_id;
    void** GIM_mem;

public:
    CXL_SHM(int num_hosts, int host_id) {
        this->num_hosts = num_hosts;
        this->host_id = host_id;
        for (int i = 0; i < num_hosts; i++) {
            int shmid;
            shmid = shmget((key_t)i, GIM_SIZE, 0666 | IPC_CREAT);
            if (shmid == -1) {
                fprintf(stderr, "shmat failed\n");
                exit(EXIT_FAILURE);
            }

            // 将共享内存连接到当前进程的地址空间
            GIM_mem[i] = shmat(shmid, 0, 0);
            if (GIM_mem[i] == (void*)-1) {
                fprintf(stderr, "shmat failed\n");
                exit(EXIT_FAILURE);
            }
            const unsigned long mask = i;
            if (mbind((void*)GIM_mem[i], GIM_SIZE, MPOL_BIND, &mask, 64, 0) != 0) {
                printf("mbind failed\n");
            }
        }
    }
};

int main(int argc, char** argv) {
    int num_hosts = 2;
    int host_id = 0;
    CXL_SHM* cxl_shm = new CXL_SHM(num_hosts, host_id);
    show_meminfo();
    return 0;
}