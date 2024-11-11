#pragma once 

#include <iostream>
#include "copyable.h"
#include <string>

class Timestamp : public copyable {
    public:
        Timestamp() : microSecondsSinceEpoch_(0) {
        }

        explicit Timestamp(int64_t microSecondsSinceEpochArg) 
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {
        }

        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds = true) const;
        // Get time of now.
        static Timestamp now(); 

        static const int kMicroSecondsPerSecond = 1000 * 1000;
    private:
        int64_t microSecondsSinceEpoch_;
};