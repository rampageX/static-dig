/* Native Windows replacements needed by BIND's time.c only. */
#ifndef STATIC_DIG_WINDOWS_TIME_COMPAT_H
#define STATIC_DIG_WINDOWS_TIME_COMPAT_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static inline struct tm *
localtime_r(const time_t *source, struct tm *out) {
    return localtime_s(out, source) == 0 ? out : NULL;
}

static inline struct tm *
gmtime_r(const time_t *source, struct tm *out) {
    return gmtime_s(out, source) == 0 ? out : NULL;
}

static inline time_t
timegm(struct tm *utc) {
    return _mkgmtime(utc);
}

/* BIND parses HTTP timestamps with the fixed RFC 1123 pattern below.
 * Deliberately reject unsupported generic strptime patterns.
 */
static inline char *
strptime(const char *input, const char *format, struct tm *result) {
    static const char *const weekdays[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char *const months[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    char weekday[4] = {0}, month[4] = {0};
    int day, year, hour, minute, second, consumed = 0;
    int weekday_id = -1, month_id = -1;
    size_t i;

    if (strcmp(format, "%a, %d %b %Y %H:%M:%S") != 0) {
        return NULL;
    }
    if (sscanf(input, "%3[A-Za-z], %2d %3[A-Za-z] %4d %2d:%2d:%2d%n",
               weekday, &day, month, &year, &hour, &minute, &second,
               &consumed) != 7 || consumed == 0) {
        return NULL;
    }
    for (i = 0; i < 7; ++i) {
        if (_stricmp(weekday, weekdays[i]) == 0) weekday_id = (int)i;
    }
    for (i = 0; i < 12; ++i) {
        if (_stricmp(month, months[i]) == 0) month_id = (int)i;
    }
    if (weekday_id < 0 || month_id < 0 || year < 1900 || day < 1 ||
        day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60) {
        return NULL;
    }
    memset(result, 0, sizeof(*result));
    result->tm_wday = weekday_id;
    result->tm_mday = day;
    result->tm_mon = month_id;
    result->tm_year = year - 1900;
    result->tm_hour = hour;
    result->tm_min = minute;
    result->tm_sec = second;
    result->tm_isdst = 0;
    return (char *)input + consumed;
}

#endif
