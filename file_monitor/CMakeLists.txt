cmake_minimum_required(VERSION 2.6)


set (CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -g -fPIC -Wall")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -g -fPIC -Wall -std=c++11")

include_directories(inc src)

aux_source_directory(src FILE_MONITOR_SRCS)

add_library(file_monitor SHARED ${FILE_MONITOR_SRCS})

target_link_libraries(file_monitor pthread)

#add_custom_command(TARGET file_monitor POST_BUILD
#    COMMAND cp -vf $<TARGET_FILE:file_monitor> ${PROJECT_SOURCE_DIR}/../../lib)
