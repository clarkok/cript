//
// Created by c on 4/5/16.
//

#include <stdio.h>

#include "CuTest.h"

CuSuite *hash_test_suite();
CuSuite *cstring_test_suite();
CuSuite *parse_test_suite();
CuSuite *young_gen_test_suite();
CuSuite *code_test_suite();
CuSuite *ir_builder_test_suite();

int
main()
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    CuSuiteAddSuite(suite, hash_test_suite());
    CuSuiteAddSuite(suite, cstring_test_suite());
    CuSuiteAddSuite(suite, parse_test_suite());
    CuSuiteAddSuite(suite, young_gen_test_suite());
    CuSuiteAddSuite(suite, code_test_suite());
    CuSuiteAddSuite(suite, ir_builder_test_suite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);

    printf("%s\n", output->buffer);

    return 0;
}
