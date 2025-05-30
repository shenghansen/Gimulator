#pragma once

// spdlog
#include "spdlog/spdlog.h"
#define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#define INFO(...) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)

// MEMINFO
#include <stdio.h>
inline void dump_memory_maps() {
    printf("Dump /proc/self/maps\n");
    int maps_fd = open("/proc/self/maps", O_RDONLY);
    if (maps_fd == -1) {
        printf("open /proc/self/maps failed\n");
        exit(-1);
    }
    char buf[1024];
    int ret = 0;
    while ((ret = read(maps_fd, buf, 1024)) > 0) {
        write(STDOUT_FILENO, buf, ret);
    }
    close(maps_fd);

    printf("Dump /proc/self/numa_maps\n");
    // print /proc/self/numa_maps to stdout
    maps_fd = open("/proc/self/numa_maps", O_RDONLY);
    if (maps_fd == -1) {
        printf("open /proc/self/maps failed\n");
        exit(-1);
    }
    while ((ret = read(maps_fd, buf, 1024)) > 0) {
        write(STDOUT_FILENO, buf, ret);
    }
    close(maps_fd);
}

// inject latency
#include <emmintrin.h>
#include <immintrin.h>
#include <x86intrin.h>
constexpr static double cycle_per_ns = 2.4;   // change by your cpu
template<int inject_ns> inline uint64_t inject_latency_ns() {
    constexpr uint64_t inject_cycle_num = inject_ns * cycle_per_ns;
    uint64_t start_c_, end_c_, temp_c_;

    start_c_ = _rdtsc();
    temp_c_ = start_c_ + inject_cycle_num;
    while ((end_c_ = _rdtsc()) < temp_c_) _mm_pause();
    return end_c_ - start_c_;
}

#define INLINE __attribute__((always_inline)) inline

