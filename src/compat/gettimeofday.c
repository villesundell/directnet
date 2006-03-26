#include "compat.h"
#include <sys/timeb.h>

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    struct timeb;
    ftime(*timeb);
    if (tv) {
        tv->tv_sec = timeb.time;
        tv->tv_usec = timeb.millitm * 1000;
    }
    if (tz) {
        assert(0 == "UNIMPLEMENTED STRUCT TIMEZONE SUPPORT");
    }
    return 0;
}
