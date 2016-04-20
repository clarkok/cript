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

int
main(int argc, const char **argv)
{
    if (argc != 2) {
        error("No source file\n");
        return -1;
    }

    FILE *source = fopen(argv[1], "r");
    fseek(source, 0, SEEK_END);
    size_t source_size = (size_t)ftell(source);
    fseek(source, 0, SEEK_SET);

    char *content = malloc(source_size + 1);
    fread(content, source_size, 1, source);
    fclose(source);

    ParseState *parse_state = parse_state_new_from_string(content);
    parse(parse_state);

    VMState *vm = cvm_state_new_from_parse_state(parse_state);

    lib_register(vm);

    cvm_state_run(vm);
    return 0;
}
