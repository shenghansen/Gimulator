#include <cerrno>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
#include <fcntl.h>
#include <hugetlbfs.h>
#include <immintrin.h>
#include <iostream>
#include <linux/mman.h>
#include <memory_resource>
#include <numa.h>
#include <numaif.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <x86intrin.h>
using namespace std;

#define POOL_SIZE 1 << 10
#ifndef NUMA_COUNT
#define NUMA_COUNT 4
#endif

enum class Type : int { REMOTE_1 = 50000, REMOTE_2 = 10000, LOCAL = 0 };

static inline uint64_t inject_cycle(uint64_t inject_cycle_num) {
  uint64_t start_c, end_c;
  uint64_t temp_c;
  start_c = _rdtsc();
  temp_c = start_c + inject_cycle_num;
  while ((end_c = _rdtsc()) < temp_c)
    _mm_pause();
  return end_c - start_c;
}

static inline uint64_t inject_cycle(Type inject_cycle_num) {
  uint64_t start_c, end_c;
  uint64_t temp_c;
  start_c = _rdtsc();
  temp_c = start_c + static_cast<int>(inject_cycle_num);
  while ((end_c = _rdtsc()) < temp_c)
    _mm_pause();
  return end_c - start_c;
}

class NUMA_Topo {
private:
  int numa_count;
  int **topo;

public:
  explicit NUMA_Topo(int numa_count, int **topo) {
    this->numa_count = numa_count;
    this->topo = topo;
  }
};

class MemPool {
private:
  int numa;
  int type;
  int capacity;
  void *index;
  int offset = 0;

public:
  MemPool(int numa, int type, int capacity,
          const std::string &gim_memory_path) { // 申请内存只在对应host完成
    this->numa = numa;
    this->type = type;
    this->capacity = capacity;
    const int flags = SHM_NORESERVE   // don't reserve swap space for this.
                      | 0666          // Leave the leading 0 (octal)
                      | SHM_R | SHM_W // These are redundant with 0666
        ;

    const int creat_flags =
        flags | IPC_CREAT // creatE a new shared memory segment
        ;

    int ret = 0;
    // actual path : /dev/shm/cxl_memory
    // const char * cxl_mem_path = "cxl_memory";
    // int shmfd = shm_open(cxl_mem_path,O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);

    // const char * huge_mem_path = "/dev/hugepages/cxl_memory";
    // const char * huge_mem_path =
    // "/var/lib/hugetlbfs/pagesize-2MB/cxl_memory"; const char * huge_mem_path
    // = "/var/lib/hugetlbfs/pagesize-1GB/cxl_memory";
    const char *huge_mem_path = gim_memory_path.c_str();

    int shmfd = open(huge_mem_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (shmfd < 0) {
      exit(-1);
    }

    bool is_pmem_dax = false;
    if (gim_memory_path.find("/dev/dax") != std::string::npos) {
      is_pmem_dax = true;
    }

    if (is_pmem_dax == false) {
      ret = ftruncate(shmfd, capacity);
      if (ret < 0) {
        exit(-1);
      }
    }

    void *start_addr = (void *)0x0000600000000000;

    // https://man7.org/linux/man-pages/man2/mmap.2.html
    const int mmap_port = PROT_READ | PROT_WRITE | PROT_EXEC;
    const int mmap_flags = MAP_FIXED | MAP_SHARED_VALIDATE | MAP_POPULATE;

    uint8_t *shm_buf =
        (uint8_t *)mmap(start_addr, capacity, mmap_port, mmap_flags, shmfd, 0);
    if (shm_buf == nullptr || shm_buf == MAP_FAILED) {
      exit(-1);
    }
    if (shm_buf != start_addr) {
      exit(-1);
    }
    unsigned long mask = 1;
    if (numa >= 0 && numa < 32) {
      mask = 1U << numa;
    } else {
      mask = 0b00000001;
    }
    mbind((void *)shm_buf, capacity, MPOL_BIND, &mask, 64, 0);
    index = shm_buf;
  }
  void *get(int size) {
    if (offset + size > capacity) {
      return nullptr;
    }

    void *ret = (char *)index + offset;
    offset += size;
    return ret;
  }
};

template <class T> class Arr {
private:
  int size;
  T *index;

public:
  Arr(int size, MemPool &mem) {
    this->size = size;
    this->index = mem.get(size);
  }
  template <int L> T &operator[](int offset) {
    inject_cycle(L);
    __m128i v = _mm_stream_load_si128((__m128i *)(index + offset));
    __sync_synchronize();
    return *(T *)&v;
  }
  T load(int offset, Type type) {
    inject_cycle(type);
    __m128i v = _mm_stream_load_si128((__m128i *)(index + offset));
    __sync_synchronize();
    return *(T *)&v;
  }
  void store(int offset, Type type, int value) {
    inject_cycle(type);
    __m128i v = _mm_stream_si32((index + offset), value);
    __sync_synchronize();
  }
};

int main(int argc, char const *argv[]) {
  int **topo = new int *[NUMA_COUNT];
  for (int i = 0; i < NUMA_COUNT; i++) {
    topo[i] = new int[NUMA_COUNT];
  }
  for (size_t i = 0; i < NUMA_COUNT; i++) {
    for (size_t j = 0; j < NUMA_COUNT; j++) {
      if (i == j)
        topo[i][j] = -1;
      topo[i][j] = 1;
    }
  }

  NUMA_Topo numa_topo(NUMA_COUNT, topo);

  /*
  每个host创建N个指针
  if id ==n
    申请自己的内存池
  N个host绑定指针到对应内存池
  if id == n
    创建自己的数据（用规定的数据结构）

   */

  return 0;
}
