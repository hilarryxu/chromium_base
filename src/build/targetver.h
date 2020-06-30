#ifndef BUILD_TARGETVER_H_
#define BUILD_TARGETVER_H_

#include "build/build_config.h"

#if !defined(OS_WIN)
#error "Preprocessing symbol 'OS_WIN' needed :-)"
#endif

#ifndef WIN32_LEAN_AND_MEAN  // remove rarely used header files, including 'winsock.h'
#define WIN32_LEAN_AND_MEAN  // which will conflict with 'winsock2.h'
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT_WINXP
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT_WINXP
#endif

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE60
#endif

#endif  // BUILD_TARGETVER_H_
