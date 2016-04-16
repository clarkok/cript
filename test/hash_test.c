//
// Created by c on 4/7/16.
//

#include "CuTest.h"
#include "hash.h"

void
hash_test_basic_insert(CuTest *tc)
{
    Hash *uut = hash_new(32);
    int i;

    for (i = 0; i < 16; ++i) {
        CuAssertIntEquals(tc, i, (int)hash_size(uut));
        hash_set(uut, (uintptr_t)i, value_from_int(i));
    }

    for (i = 0; i < 16; ++i) {
        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));
    }

    for (i = 15; i >= 0; --i) {
        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));
    }

    CuAssertTrue(tc, value_is_undefined(hash_find(uut, 16)));

    hash_destroy(uut);
}

void
hash_test_basic_remove(CuTest *tc)
{
    Hash *uut = hash_new(32);
    int i;

    for (i = 0; i < 16; ++i) {
        hash_set(uut, (uintptr_t)i, value_from_int(i));
    }

    for (i = 0; i < 16; ++i) {
        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));
    }

    for (i = 0; i < 8; ++i) {
        CuAssertIntEquals(tc, 16 - i, (int)hash_size(uut));
        hash_set(uut, (uintptr_t)i, value_undefined());
    }

    for (i = 0; i < 8; ++i) {
        CuAssertTrue(tc, value_is_undefined(hash_find(uut, (uintptr_t)i)));
    }

    for (i = 8; i < 16; ++i) {
        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));
    }

    hash_destroy(uut);
}

void
hash_test_conflict_insert(CuTest *tc)
{
    Hash *uut = hash_new(32);
    int i;

    for (i = 0; i < 16 * 32; i += 32) {
        CuAssertIntEquals(tc, i / 32, (int)hash_size(uut));
        hash_set(uut, (uintptr_t)i, value_from_int(i));
    }

    for (i = 0; i < 16 * 32; i += 32) {
        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));
    }

    hash_destroy(uut);
}

void
hash_test_conflict_remove(CuTest *tc)
{
    Hash *uut = hash_new(32);
    int i;

    for (i = 0; i < 64 * 32; i += 32) {
        CuAssertIntEquals(tc, 0, (int)hash_size(uut));
        hash_set(uut, (uintptr_t)i, value_from_int(i));
        CuAssertIntEquals(tc, 1, (int)hash_size(uut));

        CuAssertTrue(tc, value_is_int(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(uut, (uintptr_t)i)));

        hash_set(uut, (uintptr_t)i, value_undefined());
        CuAssertIntEquals(tc, 0, (int)hash_size(uut));
        CuAssertTrue(tc, value_is_undefined(hash_find(uut, (uintptr_t)i)));
    }

    hash_destroy(uut);
}

void
hash_test_expand(CuTest *tc)
{
    Hash *uut = hash_new(32);
    int i;

    for (i = 0; i < 16; ++i) {
        hash_set(uut, (uintptr_t)i, value_from_int(i));
    }
    CuAssertTrue(tc, !hash_need_expand(uut));
    for (i = 16; i < 30; ++i) {
        hash_set(uut, (uintptr_t)i, value_from_int(i));
    }
    CuAssertTrue(tc, hash_need_expand(uut));

    Hash *expanded = hash_expand(uut);

    CuAssertTrue(tc, hash_capacity(expanded) > hash_capacity(uut));
    CuAssertTrue(tc, hash_size(expanded) == hash_size(uut));

    for (i = 30; i < 36; ++i) {
        hash_set(expanded, (uintptr_t)i, value_from_int(i));

        CuAssertTrue(tc, value_is_undefined(hash_find(uut, (uintptr_t)i)));
        CuAssertIntEquals(tc, i, (int)value_to_int(hash_find(expanded, (uintptr_t)i)));
    }

    hash_destroy(expanded);
    hash_destroy(uut);
}

CuSuite *
hash_test_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, hash_test_basic_insert);
    SUITE_ADD_TEST(suite, hash_test_basic_remove);
    SUITE_ADD_TEST(suite, hash_test_conflict_insert);
    SUITE_ADD_TEST(suite, hash_test_conflict_remove);
    SUITE_ADD_TEST(suite, hash_test_expand);

    return suite;
}