#pragma once 

#include <string>
#include "noncopyable.h"
#include "Timestamp.h"
#include <stdlib.h>
/**
 * LOG_INFO("%s %d", arg1, arg2)
 * ... 是一个可变参数宏(variadic macro)的写法，允许这个宏接受不定数量的参数。
 * __VA_ARGS__ 是一个预定义的宏，代表传递给...的实际参数。前面的##作用是去除可变参数为空的情况下
 * ##__VA_ARGS__会去除多余的逗号。 
 * */

#define LOG_INFO(logmsgFormat, ...)                         \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::instance();                \
        logger.setLogLevel(INFO);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0)                                             \
    
#define LOG_ERROR(logmsgFormat, ...)                         \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::instance();                \
        logger.setLogLevel(ERROR);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0)                                             \

#define LOG_FATAL(logmsgFormat, ...)                         \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::instance();                \
        logger.setLogLevel(FATAL);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
        exit(-1);                                           \
    } while (0)                                             \

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                         \
    do                                                      \
    {                                                       \
        Logger& logger = Logger::instance();                \
        logger.setLogLevel(DEBUG);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while (0)                                             \
#else 
    #define LOG_DEBUG(logmsgFormat, ...)                        
#endif

// 定义日志的级别 INFO ERROR FATAL DEBUG
enum LogLevel {
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core信息
    DEBUG,  // 调试信息
};

// 输出一个日志类
class Logger : noncopyable {
    public:
        // 获取日志唯一的实例对象
        static Logger& instance();

        // 设置日志级别 
        void setLogLevel(int level);
        // 写日志
        void log(std::string msg);
    private:
        int logLevel_;
        Logger() {}

};