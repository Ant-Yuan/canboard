project(canboard)

add_executable(canboard main.c canBsp.c logBsp.c socketBsp.c)
find_package (Threads)
target_link_libraries (canboard ${CMAKE_THREAD_LIBS_INIT})


install(TARGETS canboard RUNTIME DESTINATION bin)
