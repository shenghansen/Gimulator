#include "CXL_SHM.h"
#include "utills.hpp"
#include <cstddef>
#include <iterator>
#include <smmintrin.h>

template<class T, bool R , int LATENCY> class GIM_array {
private:
    size_t capacity;
    size_t size;
    T* data;
    int host_id;
    CXL_SHM* cxl_shm;
    

public:
    GIM_array(size_t size, const CXL_SHM* cxl_shm);
    ~GIM_array();
    T& operator[](size_t offset);
    GIM_array& operator=(const GIM_array& a);
    GIM_array& operator=(T a);
    size_t get_size();
};