//
// Created by c on 4/4/16.
//

#include <stdio.h>
#include <stdlib.h>

#include "cript.h"
#include "cvm.h"
#include "error.h"

#include "libs.h"

#include "inst_output.h"

static void
output_help()
{
    printf(
        "Cript %s\n"
        "Usage:"
        "  cript [options] source.cr\n"
        "\n"
        "Options:"
        "  -c\tprint compiled ir before execute\n"
        "  -h\tprint this help message and exit\n"
        "  -v\tprint current version and exit\n",
        CRIPT_VERSION
    );
}

int
main(int argc, const char **argv)
{
    const char *source = NULL;
    int output_insts = 0;

    --argc; ++argv;
    while (argc) {
        const char *arg = *argv;
        if (arg[0] == '-') {
            switch (arg[1]) {
                case 'v':
                {
                    printf(
                        "Cript version %s\n"
                        "Clarkok Zhang(mail@clarkok.com)\n",
                        CRIPT_VERSION
                    );
                    return 0;
                }
                case 'c':
                {
                    output_insts = 1;
                    break;
                }
                case 'h':
                    output_help();
                    return 0;
                default:
                {
                    printf(
                        "Unknown argument: %s\n",
                        arg
                    );
                    output_help();
                    return -1;
                }
            }
        }
        else {
            if (source) {
                error("Multiple source file\n  Use import instead\n");
                return -1;
            }
            else {
                source = arg;
            }
        }
        --argc; ++argv;
    }

    if (!source) {
        error("No source file\n");
        return -1;
    }
    ParseState *parse_state = parse_state_new_from_file(source);
    parse(parse_state);

    VMState *vm = cvm_state_new_from_parse_state(parse_state);

    if (output_insts) {
        output_vm_state(stdout, vm);
    }

    lib_register(vm);
    cvm_state_run(vm);

    return 0;
}
