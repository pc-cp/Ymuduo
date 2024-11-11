#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                const std::string& name) 
: loop_(nullptr)
, exiting_(false)
, thread_(std::bind(&EventLoopThread::threadFunc, this), name)
, mutex_()
, cv_()
, callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    if(loop_ != nullptr) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    thread_.start(); // 运行底层的新线程

    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr) {
            cv_.wait(lock);
        }
        loop = loop_;
    }
    
    return loop;
}

/**
 * 下面这个方法，是在单独的新线程里面运行的！！ 非常牛逼
 */
void EventLoopThread::threadFunc() {
    EventLoop loop;     // 创建一个独立的EventLoop，和上面的线程是一一对应的， one loop per thread

    if(callback_) {
        callback_(&loop);
    }

    {
        // 线程间通信
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cv_.notify_one();
    }

    loop.loop();        // EventLoop -> Poller.poll

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}