/* The BIND QP allocator uses only anonymous mmap/mprotect/munmap.
 * Reuse the native VirtualAlloc wrapper built for Userspace RCU.
 */
#ifndef STATIC_DIG_BIND_WINDOWS_MMAN_H
#define STATIC_DIG_BIND_WINDOWS_MMAN_H
#include "../../urcu-compat/sys/mman.h"
#endif
