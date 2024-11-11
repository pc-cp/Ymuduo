#include "Timestamp.h"
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>

std::string Timestamp::toString() const {
    char buf[128] = {0};
    int64_t seconds = microSecondsSinceEpoch_/kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_%kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

std::string Timestamp::toFormattedString(bool showMicroseconds) const {
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    if(showMicroseconds) {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_%kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

        // Get time of now.
Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return Timestamp(tv.tv_sec * kMicroSecondsPerSecond + tv.tv_usec);
}


// #include <iostream>
// int main() {
//     std::cout << Timestamp::now().toFormattedString() << std::endl;
// }