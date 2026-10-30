/* Native Windows AF_UNIX structure compatibility for BIND headers.
 * Windows 10+ SDKs provide this in afunix.h, not sys/un.h.
 */
#ifndef STATIC_DIG_WINDOWS_SYS_UN_H
#define STATIC_DIG_WINDOWS_SYS_UN_H

#include <winsock2.h>

#if defined(__has_include)
# if __has_include(<afunix.h>)
#  include <afunix.h>
#  define STATIC_DIG_AFUNIX_SDK 1
# endif
#endif

#if !defined(STATIC_DIG_AFUNIX_SDK)
# if !defined(AF_UNIX)
#  define AF_UNIX 1
# endif
struct sockaddr_un {
    ADDRESS_FAMILY sun_family;
    char sun_path[108];
};
#endif

#endif
