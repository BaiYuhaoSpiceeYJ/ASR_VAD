cmake_minimum_required(VERSION 3.3.2)
project(vad)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall")



set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(online_vad main_test.c vad.c online_vad.c cJSON.cc)

add_executable(audio_segment main_segment_audio.c vad.c online_vad.c cJSON.cc)



add_library(sub_lib_shared  SHARED vad.c online_vad.c cJSON.cc)
set_target_properties(sub_lib_shared PROPERTIES OUTPUT_NAME "online-vad")
set_target_properties(sub_lib_shared PROPERTIES VERSION 1.0 SOVERSION 1)

add_library(sub_lib_static STATIC vad.c online_vad.c cJSON.cc)
set_target_properties(sub_lib_static PROPERTIES OUTPUT_NAME "online-vad")


MESSAGE(STATUS "lib is installed in  dir: " ${CMAKE_INSTALL_PREFIX})

INSTALL(TARGETS sub_lib_shared sub_lib_static
    LIBRARY DESTINATION Online_VAD/lib
    ARCHIVE DESTINATION Online_VAD/lib)

#安装头文件
INSTALL(FILES "online_vad.h"  DESTINATION Online_VAD/include)
