#include "CXL_SHM.h"
#include "utills.hpp"
#include <cstddef>
#include <iterator>

//Primitive data type wrappers for gim latency
template<class T, bool R,int LATENCY> class GIM_data {
public:
     T data;
    GIM_data(T data);
    ~GIM_data(){};//memory free by GIM_array->GIM_mem
    GIM_data& operator=(const GIM_data<T, R,LATENCY>& a);
    GIM_data& operator=(T a);


};
