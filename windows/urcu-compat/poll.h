/* Narrow Windows compatibility shim for Userspace RCU 0.15.7.
 * All uses in src/ and include/ call poll(NULL, 0, timeout) as a timed yield.
 * Socket polling is deliberately not implemented.
 */
#ifndef STATIC_DIG_URCU_POLL_H
#define STATIC_DIG_URCU_POLL_H
#include <errno.h>
#include <windows.h>
static inline int poll(void *fds, unsigned long nfds, int timeout_ms)
{
    if (fds != NULL || nfds != 0) {
        errno = EINVAL;
        return -1;
    }
    Sleep(timeout_ms < 0 ? INFINITE : (DWORD)timeout_ms);
    return 0;
}
#endif
