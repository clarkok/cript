cmake_minimum_required(VERSION 2.8)
project(cript)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")

set(LIB_SRC_FILES src/cript.h src/cvm.c src/cvm.h src/list.h src/error.c src/error.h src/object.c src/object.h src/value.c src/value.h src/hash.c src/hash.h src/string_pool.c src/string_pool.h src/hash_helper.h src/inst.h src/inst_list.c src/inst_list.h src/parse.c src/parse.h src/parse_internal.h src/inst_output.h src/inst_output.c)
set(SOURCE_FILES src/cript.c)

add_library(libcript ${LIB_SRC_FILES})
add_executable(cript ${SOURCE_FILES})
target_link_libraries(cript libcript)

add_subdirectory(test)