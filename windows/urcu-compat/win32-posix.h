/* Narrow native-Windows shims for Userspace RCU 0.15.7.
 * Only for the experimental MinGW build, not a general POSIX layer.
 */
#ifndef STATIC_DIG_URCU_WIN32_POSIX_H
#define STATIC_DIG_URCU_WIN32_POSIX_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <windows.h>

#ifdef _WIN32
/* Win32 does not support POSIX signal inheritance in pthread_create. */
typedef int sigset_t;
static inline int sigfillset(sigset_t *set)
{
    *set = 0;
    return 0;
}
#define pthread_sigmask(how, set, oldset) (0)

/* UCRT malloc is aligned to at least 16 bytes on x64. Return an error
 * instead of an incorrectly aligned pointer for larger alignments.
 * This API is currently not invoked by the liburcu default backend.
 */
static inline int posix_memalign(void **out, size_t alignment, size_t size)
{
    void *p;
    if (!out || alignment == 0 || (alignment & (alignment - 1)) != 0)
        return EINVAL;
    if (alignment > 16) return ENOMEM;
    p = malloc(size);
    if (!p) return ENOMEM;
    if (((uintptr_t)p & (alignment - 1)) != 0) {
        free(p);
        return ENOMEM;
    }
    *out = p;
    return 0;
}

static inline int getpagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwPageSize;
}
#endif

#endif
