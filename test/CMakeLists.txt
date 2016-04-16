enable_testing()

include_directories(${cript_SOURCE_DIR}/src)
include_directories(${cript_SOURCE_DIR}/test/cutest-1.5)

add_executable(test_main cutest-1.5/CuTest.c test_main.c cvm_test.c hash_test.c cstring_test.c)
target_link_libraries(test_main libcript)
