//
// Created by c on 4/18/16.
//

#include "CuTest.h"

#include "young_gen.h"

void
young_gen_basic_test(CuTest *tc)
{
    YoungGen *young_gen = young_gen_new();

    Hash *root = young_gen_new_hash(young_gen, HASH_MIN_CAPACITY, HT_OBJECT);

    for (int i = 0; i < 8; ++i) {
        Hash *child = young_gen_new_hash(young_gen, HASH_MIN_CAPACITY, HT_OBJECT);
        hash_set(root, (uintptr_t)i, value_from_ptr(child));
    }

    size_t heap_size = young_gen_heap_size(young_gen);
    Hash *root_old = root;

    for (int i = 0; i < 8; ++i) {
        hash_set(root, (uintptr_t)i, value_null());
    }

    young_gen_gc_start(young_gen);
    young_gen_gc_mark(young_gen, &root);
    young_gen_gc_end(young_gen);

    CuAssertTrue(tc, heap_size > young_gen_heap_size(young_gen));
    CuAssertTrue(tc, root_old != root);

    young_gen_destroy(young_gen);
}

void
young_gen_data_after_gc_test(CuTest *tc)
{
    YoungGen *young_gen = young_gen_new();

    Hash *root = young_gen_new_hash(young_gen, HASH_MIN_CAPACITY, HT_OBJECT);

    for (int i = 0; i < 8; ++i) {
        hash_set(root, (uintptr_t)i, value_from_int(i));
    }

    young_gen_gc_start(young_gen);
    young_gen_gc_mark(young_gen, &root);
    young_gen_gc_end(young_gen);

    for (int i = 0; i < 8; ++i) {
        CuAssertIntEquals(tc, i, value_to_int(hash_find(root, (uintptr_t)i)));
    }

    young_gen_destroy(young_gen);
}

CuSuite *
young_gen_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, young_gen_basic_test);
    SUITE_ADD_TEST(suite, young_gen_data_after_gc_test);

    return suite;
}
