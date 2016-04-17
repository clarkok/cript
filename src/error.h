//
// Created by c on 4/4/16.
//

#ifndef CRIPT_ERROR_H
#define CRIPT_ERROR_H

#define error_f(fmt, ...)   error_handle_f(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define error(msg)          error_handle(__FILE__, __LINE__, msg);

#define warn_f(fmt, ...)    warn_handle_f(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define warn(msg)           warn_handle(__FILE__, __LINE__, msg);

void error_handle_f(const char *file, int line, const char *fmt, ...);
void error_handle(const char *file, int line, const char *msg);

void warn_handle_f(const char *file, int line, const char *fmt, ...);
void warn_handle(const char *file, int line, const char *msg);

#endif //CRIPT_ERROR_H
