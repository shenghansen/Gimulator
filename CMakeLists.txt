cmake_minimum_required(VERSION 3.22)
project( GIMULATOR )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(DML_DIR ~/DML/install/lib/cmake/DML)

find_package(OpenMP REQUIRED)
find_package(DML REQUIRED)
find_package(MPI REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DSTATISTICS -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add debug flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "-march=native  ${CMAKE_CXX_FLAGS}")


include_directories(include)
include_directories(${MPI_INCLUDE_PATH})
include_directories(~/DML/install/include)

add_executable(demo test/test_dma.cpp src/DMA.cpp src/CXL_SHM.cpp)
target_link_libraries(demo numa dml dmlhl OpenMP::OpenMP_CXX ${MPI_LIBRARIES})
# target_compile_definitions(demo PRIVATE STATISTICS SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)


add_executable(gim test/test_gim.cpp src/CXL_SHM.cpp src/DMA.cpp src/communicate.cpp)
target_link_libraries(gim numa dml dmlhl OpenMP::OpenMP_CXX ${MPI_LIBRARIES})
# target_compile_definitions(gim PRIVATE STATISTICS SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

add_executable(probe test/test_probe.cpp src/CXL_SHM.cpp src/DMA.cpp src/communicate.cpp)
target_link_libraries(probe numa dml dmlhl OpenMP::OpenMP_CXX ${MPI_LIBRARIES})
# target_compile_definitions(gim PRIVATE STATISTICS SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
