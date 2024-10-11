cmake_minimum_required(VERSION 3.22)
project( GIMULATOR )
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(OpenMP REQUIRED)
set(CMAKE_CXX_STANDARD 17)


add_definitions(-DSTATISTICS)

set(CMAKE_CXX_FLAGS "-march=native  ${CMAKE_CXX_FLAGS}")
include_directories(include)
add_executable(demo main.cpp)
target_link_libraries(demo numa dml OpenMP::OpenMP_CXX)



# add_definitions(-DHIGH_PERFORMANCE)
# add_definitions(-DPREFETCH)


# include_directories(include)
# include_directories(/mnt/nvme0/home/shs/brpc/output/include)
# link_directories(/mnt/nvme0/home/shs/brpc/output/lib)

# set(DATACENTER_SRCS
#   ${CMAKE_SOURCE_DIR}/src/datacenterKV.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/leveldb.cpp
#   ${CMAKE_SOURCE_DIR}/src/datacenter/server.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/KVService.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/UpdateClient.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/HttpService.pb.cc
# )

# set(EDGE_SRCS
#   ${CMAKE_SOURCE_DIR}/src/edgeKV.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/leveldb.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/client.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/EdgeService.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/S3FIFO.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/KVService.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/UpdateClient.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/HttpService.pb.cc

# )
# set(test_DML/test_EDGE_SRCS
#   ${CMAKE_SOURCE_DIR}/test_DML/test/test_DML/test_edgeKV.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/leveldb.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/client.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/EdgeService.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/S3FIFO.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/KVService.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/UpdateClient.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/HttpService.pb.cc

# )

# set(test_DML/test_CLIENT
# ${CMAKE_SOURCE_DIR}/test_DML/test/test_DML/test_client.cpp 
#   ${CMAKE_SOURCE_DIR}/src/utill/leveldb.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/client.cpp
#   ${CMAKE_SOURCE_DIR}/src/edge/S3FIFO.cpp
#   ${CMAKE_SOURCE_DIR}/src/utill/KVService.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/UpdateClient.pb.cc
#   ${CMAKE_SOURCE_DIR}/src/utill/HttpService.pb.cc
# )

# add_executable(datacenterKV  ${DATACENTER_SRCS})
# add_executable(edgeKV ${EDGE_SRCS})
# add_executable(test_DML/test_edgeKV ${test_DML/test_EDGE_SRCS})
# add_executable(test_DML/testleveldb test_DML/test/test_DML/test_leveldb.cpp src/utill/leveldb.cpp)
# add_executable(test_DML/testclient ${test_DML/test_CLIENT})


# target_link_libraries(edgeKV spdlog leveldb snappy brpc protobuf gflags)
# target_link_libraries(test_DML/test_edgeKV spdlog leveldb snappy brpc protobuf gflags)
# target_link_libraries(datacenterKV spdlog leveldb snappy brpc protobuf gflags)
# target_link_libraries(test_DML/testleveldb leveldb fmt)
# target_link_libraries(test_DML/testclient leveldb fmt spdlog leveldb snappy brpc protobuf gflags)