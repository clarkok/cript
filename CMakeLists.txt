cmake_minimum_required(VERSION 3.4)
project(cript)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/cript.c src/cript.h src/cvm.c src/cvm.h src/list.c src/list.h src/error.c src/error.h src/object.c src/object.h src/value.c src/value.h)
add_executable(cript ${SOURCE_FILES})

add_subdirectory(test)