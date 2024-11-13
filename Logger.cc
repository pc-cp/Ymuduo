#include "Logger.h"
#include <iostream>
#include "string.h"

__thread char t_errnobuf[512];
__thread char t_time[64];

const char* strerror_tl(int savedErrno) {
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

// 设置日志级别 
void Logger::setLogLevel(int level) {
    logLevel_ = level;
}

// 写日志 [级别信息] time : msg
void Logger::log(std::string msg) {
    switch(logLevel_) {
        case INFO:
            std::cout << "[INFO]";
            break;
        case ERROR:
            std::cout << "[ERROR]";
            break;
        case FATAL:
            std::cout << "[FATAL]";
            break;
        case DEBUG:
            std::cout << "[DEBUG]";
            break;
        default:
            break;
    }
    // 打印时间和msg
    std::cout << Timestamp::now().toFormattedString() << " : " << msg << std::endl;
}