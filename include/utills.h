#ifndef UTILLS_H
#define UTILLS_H

// spdlog
#include "spdlog/spdlog.h"
#define DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#define INFO(...) SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)

// MEMINFO
#include <stdio.h>
void show_meminfo() {
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






#endif