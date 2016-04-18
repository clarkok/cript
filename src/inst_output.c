//
// Created by c on 4/17/16.
//

#include <assert.h>

#include "inst_output.h"

const char INST_NAME[] =
    "unknown "
    "halt    "
    "li      "
    "add     "
    "sub     "
    "mul     "
    "div     "
    "mod     "
    "seq     "
    "slt     "
    "sle     "

    "lnot    "

    "bnr     "
    "j       "
    "jal     "

    "lstr    "
;

static_assert(8 * INST_NR + 1 == sizeof(INST_NAME), "Keep INST_NAME synced with insts");
