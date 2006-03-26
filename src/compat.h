#ifndef COMPAT_H
#define COMPAT_H 1

// Compatibility functions, for win32 and friends

#include "config.h"
#include <time.h>

#ifdef WIN32
#include <winsock.h> // for struct timeval, of all things

struct timezone { } // STUB
#endif

#ifndef HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

#endif

