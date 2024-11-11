#include "DMA.hpp"
#include <iostream>

int main(int argc, char** argv) {
    size_t size = std::stoull(argv[1]);
    uint8_t* source = new uint8_t[size];
    uint8_t* destination = new uint8_t[size];
    // if (mlock(source, size) != 0) {
    //     std::cerr << "Failed to lock source memory!" << std::endl;
    //     free(source);
    //     return -1;
    // }
    // if (mlock(destination, size) != 0) {
    //     std::cerr << "Failed to lock destination memory!" << std::endl;
    //     free(destination);
    //     return -1;
    // }
#pragma omp parallel for
    for (size_t i = 0; i < size; i++) {
        source[i] = i % 256;
    }
    DMA_memcpy(source, destination, size, 1);
    // hl_DMA_memcpy(source, destination, size);
    // hl_DMA_memcpy_asynchronous(source, destination, size);
}