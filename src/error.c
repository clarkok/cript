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
    fprintf(stderr, "ERROR: %s\n    thrown at %s:%d\n\n", msg, file, line);
    __builtin_trap();
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

void
warn_handle(const char *file, int line, const char *msg)
{
    fprintf(stderr, "WARN: %s\n    thrown at %s:%d\n\n", msg, file, line);
}

void
warn_handle_f(const char *file, int line, const char *fmt, ...)
{
    char msg[MESSAGE_MAXIMUM_LENGTH];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, MESSAGE_MAXIMUM_LENGTH, fmt, args);
    warn_handle(file, line, msg);
}

void
info_handle(const char *file, int line, const char *msg)
{
    // fprintf(stdout, "INFO: %s\n    thrown at %s:%d\n\n", msg, file, line);
}

void
info_handle_f(const char *file, int line, const char *fmt, ...)
{
    char msg[MESSAGE_MAXIMUM_LENGTH];
    va_list args;

    va_start(args, fmt);
    vsnprintf(msg, MESSAGE_MAXIMUM_LENGTH, fmt, args);
    info_handle(file, line, msg);
}
