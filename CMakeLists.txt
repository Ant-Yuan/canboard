 cmake_minimum_required(VERSION 2.8)
 project(canboard)
 
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_FIND_ROOT_PATH "/home/dyj/canboard/sama5/source/linux-at91-linux4sam_4.7")
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_C_COMPILER "/home/dyj/canboard/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc")




add_executable(canboard main.c canBsp.c logBsp.c socketBsp.c sysManager.c)
find_package (Threads)
target_link_libraries (canboard ${CMAKE_THREAD_LIBS_INIT})


install(TARGETS canboard RUNTIME DESTINATION bin)
