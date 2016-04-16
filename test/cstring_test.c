//
// Created by c on 4/16/16.
//

#include "CuTest.h"
#include "string_pool.h"

void
cstring_test_construct(CuTest *tc)
{
    StringPool *uut = string_pool_new();

    string_pool_destroy(uut);

    (void)tc;
}

void
cstring_test_insert(CuTest *tc)
{
    static const char TEST_STR[] = "Hello World";
    static const char TEST_STR2[] = "Hello Cript";

    StringPool *uut = string_pool_new();

    CString *inserted = string_pool_insert_str(&uut, TEST_STR);
    CuAssertIntEquals(tc, 0, strncmp(TEST_STR, inserted->content, inserted->length));

    inserted = string_pool_insert_vec(&uut, TEST_STR2, strlen(TEST_STR2));
    CuAssertIntEquals(tc, 0, strncmp(TEST_STR2, inserted->content, inserted->length));

    string_pool_destroy(uut);
}

void
cstring_test_find(CuTest *tc)
{
    static char TEST_STR[] = "Hello World";
    static char TEST_STR2[] = "Hello Cript";

    StringPool *uut = string_pool_new();

    string_pool_insert_str(&uut, TEST_STR);
    string_pool_insert_str(&uut, TEST_STR2);

    CString *find_result;

    find_result = string_pool_find_str(uut, TEST_STR);
    CuAssertTrue(tc, find_result != NULL);
    CuAssertIntEquals(tc, 0, strncmp(TEST_STR, find_result->content, strlen(TEST_STR)));

    find_result = string_pool_find_str(uut, TEST_STR2);
    CuAssertTrue(tc, find_result != NULL);
    CuAssertIntEquals(tc, 0, strncmp(TEST_STR2, find_result->content, strlen(TEST_STR2)));

    string_pool_destroy(uut);
}

void
cstring_test_rehash(CuTest *tc)
{
    StringPool *uut = string_pool_new();

    int i;
    for (i = 0; i < 16; ++i) {
        CString *inserted = string_pool_insert_vec(&uut, (char*)&i, sizeof(int));
        CuAssertIntEquals(tc, i, *(int*)inserted->content);
    }

    for (i = 0; i < 16; ++i) {
        CString *find_result = string_pool_find_vec(uut, (char*)&i, sizeof(int));
        CuAssertTrue(tc, find_result != NULL);
        CuAssertIntEquals(tc, i, *(int*)find_result->content);
    }

    string_pool_destroy(uut);
}

void
cstring_test_new_page(CuTest *tc)
{
    static char TEST_STR[255];

    StringPool *uut = string_pool_new();

    int i;
    for (i = 0; i < 16; ++i) {
        *(int*)TEST_STR = i;
        CString *inserted = string_pool_insert_vec(&uut, TEST_STR, 255);
        CuAssertIntEquals(tc, i, *(int*)inserted->content);
    }

    for (i = 0; i < 16; ++i) {
        *(int*)TEST_STR = i;
        CString *find_result = string_pool_find_vec(uut, TEST_STR, 255);
        CuAssertIntEquals(tc, i, *(int*)find_result->content);
    }

    string_pool_destroy(uut);
}

void
cstring_test_same_node(CuTest *tc)
{
    StringPool *uut = string_pool_new();

    CString *results[16];

    int i;
    for (i = 0; i < 16; ++i) {
        results[i] = string_pool_insert_vec(&uut, (char*)&i, sizeof(int));
    }

    for (i = 0; i < 16; ++i) {
        CuAssertTrue(tc, results[i] == string_pool_insert_vec(&uut, (char*)&i, sizeof(int)));
    }

    string_pool_destroy(uut);
}

void
cstring_test_empty_string(CuTest *tc)
{
    StringPool *uut = string_pool_new();

    CString *results[16];

    int i;
    for (i = 1; i < 16; ++i) {
        results[i] = string_pool_insert_vec(&uut, (char*)&i, sizeof(int));
    }

    results[0] = string_pool_insert_vec(&uut, (char*)&i, 0);

    for (i = 1; i < 16; ++i) {
        CuAssertTrue(tc, results[i] == string_pool_insert_vec(&uut, (char*)&i, sizeof(int)));
    }

    CuAssertTrue(tc, results[0] == string_pool_find_vec(uut, NULL, 0));

    string_pool_destroy(uut);
}

CuSuite *
cstring_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, cstring_test_construct);
    SUITE_ADD_TEST(suite, cstring_test_insert);
    SUITE_ADD_TEST(suite, cstring_test_find);
    SUITE_ADD_TEST(suite, cstring_test_rehash);
    SUITE_ADD_TEST(suite, cstring_test_new_page);
    SUITE_ADD_TEST(suite, cstring_test_same_node);
    SUITE_ADD_TEST(suite, cstring_test_empty_string);

    return suite;
}
