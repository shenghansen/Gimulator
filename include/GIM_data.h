#include "CXL_SHM.h"
#include "utills.hpp"
#include <cstddef>
#include <iterator>

#define LATENCY 300

template<class T, bool R> class Data {
private:
    T& data;

public:
    Data& operator=(const Data<T, true>& a);
    Data& operator=(const Data<T, false>& a);
};
