/* Types only: this header is forced into every BIND Autoconf compiler test.
 * Do not include C library headers here; they alter detection of builtins.
 */
#ifndef STATIC_DIG_WINDOWS_LEGACY_TYPES_H
#define STATIC_DIG_WINDOWS_LEGACY_TYPES_H
typedef unsigned int uint;
typedef unsigned short in_port_t;
typedef unsigned short sa_family_t;
#endif
