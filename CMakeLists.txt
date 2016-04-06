cmake_minimum_required(VERSION 2.8)
project(cript)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

set(LIB_SRC_FILES src/cript.h src/cvm.c src/cvm.h src/list.c src/list.h src/error.c src/error.h src/object.c src/object.h src/value.c src/value.h)
set(SOURCE_FILES src/cript.c)

add_library(libcript ${LIB_SRC_FILES})
add_executable(cript ${SOURCE_FILES})
target_link_libraries(cript libcript)

add_subdirectory(test)