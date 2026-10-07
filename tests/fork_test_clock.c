// SPDX-License-Identifier: GPL-3.0-or-later
// Process-local wall clock adjustment for the midnight regression.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <time.h>

static time_t offset;

static int real_clock_gettime(clockid_t clock, struct timespec* value)
{
    static int (*original)(clockid_t, struct timespec*);
    if (!original) {
        original = dlsym(RTLD_NEXT, "clock_gettime");
    }
    return original(clock, value);
}

void setForkTestTime(int64_t epoch)
{
    struct timespec now;
    real_clock_gettime(CLOCK_REALTIME, &now);
    offset = (time_t)epoch - now.tv_sec;
}

int clock_gettime(clockid_t clock, struct timespec* value)
{
    int result = real_clock_gettime(clock, value);
    if (!result && clock == CLOCK_REALTIME) {
        value->tv_sec += offset;
    }
    return result;
}
