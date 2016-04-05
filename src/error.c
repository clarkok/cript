//
// Created by c on 4/4/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "error.h"

#define MESSAGE_MAXIMUM_LENGTH  1024

void
error_handle(const char *file, int line, const char *msg)
{
    fprintf(stderr, "%s\n    thrown at %s:%d\n\n", msg, file, line);
    exit(-1);
}

void
error_handle_f(const char *file, int line, const char *fmt, ...)
{
    char msg[MESSAGE_MAXIMUM_LENGTH];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, MESSAGE_MAXIMUM_LENGTH, fmt, args);
    error_handle(file, line, msg);
}

