#pragma once 
#include "Thread.h"
#include "noncopyable.h"
#include <mutex>
#include <condition_variable>
#include <mutex>
#include <condition_variable>
class EventLoop;

class EventLoopThread : noncopyable {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;
        // cb 是默认为null
        EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());
        ~EventLoopThread();

        EventLoop* startLoop();

    private:
        void threadFunc();

        EventLoop* loop_;
        bool exiting_;
        Thread thread_;
        std::mutex mutex_;
        std::condition_variable cv_;
        ThreadInitCallback callback_;
};