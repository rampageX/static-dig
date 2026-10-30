/* Windows VirtualAlloc-backed mmap subset for liburcu 0.15.7 only.
 * Supports anonymous reservation, fixed commit/decommit and release.
 * It is not a general POSIX mmap replacement.
 */
#ifndef STATIC_DIG_URCU_WINDOWS_MMAN_H
#define STATIC_DIG_URCU_WINDOWS_MMAN_H

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <windows.h>

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 2
#define MAP_FIXED 16
#define MAP_ANONYMOUS 32
#define MAP_ANON MAP_ANONYMOUS
#define MAP_FAILED ((void *)-1)

static inline void *mmap(void *addr, size_t length, int prot, int flags,
                         int fd, off_t offset)
{
    void *result;
    DWORD allocation;
    (void)fd;
    (void)offset;
    if (!(flags & MAP_ANONYMOUS) || length == 0) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    if ((flags & MAP_FIXED) && addr && prot == PROT_NONE) {
        if (VirtualFree(addr, length, MEM_DECOMMIT)) return addr;
        errno = EINVAL;
        return MAP_FAILED;
    }
    allocation = (flags & MAP_FIXED) ? MEM_COMMIT :
                 (prot == PROT_NONE ? MEM_RESERVE : MEM_RESERVE | MEM_COMMIT);
    result = VirtualAlloc(addr, length, allocation,
                          prot == PROT_NONE ? PAGE_NOACCESS : PAGE_READWRITE);
    if (!result || ((flags & MAP_FIXED) && result != addr)) {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    return result;
}

static inline int munmap(void *addr, size_t length)
{
    (void)length;
    if (VirtualFree(addr, 0, MEM_RELEASE)) return 0;
    errno = EINVAL;
    return -1;
}

static inline int mprotect(void *addr, size_t length, int prot)
{
    DWORD old_protection;
    if (VirtualProtect(addr, length,
                       prot == PROT_NONE ? PAGE_NOACCESS : PAGE_READWRITE,
                       &old_protection)) return 0;
    errno = EINVAL;
    return -1;
}

#endif
