//
// Created by c on 4/4/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "cript.h"
#include "cvm.h"

#define ASSERT_EQ(expect, real)                 \
    do {                                        \
        if (expect != real) {                   \
            fprintf(stderr,                     \
                "Assert Failed! %s != %s\n"     \
                "  expect %d, while met %d",    \
                #expect, #real, expect, real    \
            );                                  \
            exit(-1);                           \
        }                                       \
    } while (0)

int
main(int argc, const char **argv)
{
    printf("Hello Cript!\nVersion %s\n", CRIPT_VERSION);

    InstList *insts = cvm_list_new(16);
    insts = cvm_list_append(insts, cvm_inst_new_i_type(I_LI, 1, 5));
    insts = cvm_list_append(insts, cvm_inst_new_i_type(I_LI, 2, 10));
    insts = cvm_list_append(insts, cvm_inst_new_d_type(I_ADD, 3, 1, 2));
    insts = cvm_list_append(insts, cvm_inst_new_d_type(I_HALT, 0, 0, 0));

    VMState *vm = cvm_state_new(insts);
    cvm_state_run(vm);

    ASSERT_EQ(5, value_to_int(vm->regs[1]));
    ASSERT_EQ(10, value_to_int(vm->regs[2]));
    ASSERT_EQ(15, value_to_int(vm->regs[3]));

    cvm_state_destroy(vm);

    return 0;
}
