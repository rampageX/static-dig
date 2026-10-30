/* POSIX strerror_r() signature backed by the thread-safe UCRT strerror_s. */
#ifndef STATIC_DIG_WINDOWS_STRING_COMPAT_H
#define STATIC_DIG_WINDOWS_STRING_COMPAT_H
#include <string.h>

static inline int
strerror_r(int code, char *buffer, size_t capacity) {
    return (int)strerror_s(buffer, capacity, code);
}

#endif
