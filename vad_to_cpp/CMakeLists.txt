cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(example_torch)
set(CMAKE_PREFIX_PATH "/data/test_pytorch_vad/libtorch")

find_package(Torch REQUIRED)
add_executable(${PROJECT_NAME} "test_vad.cpp")
target_link_libraries(${PROJECT_NAME} ${TORCH_LIBRARIES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
