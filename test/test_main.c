//
// Created by c on 4/5/16.
//

#include <stdio.h>

#include "CuTest.h"

CuSuite *cvm_test_suite();

int
main()
{
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    CuSuiteAddSuite(suite, cvm_test_suite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);

    printf("%s\n", output->buffer);

    return 0;
}