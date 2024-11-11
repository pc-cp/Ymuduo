#pragma once

#include "noncopyable.h"
#include <functional>
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>
#include <mutex>
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类
// Reactor， at most one per thread.
class EventLoop : noncopyable {
    public:
        using Functor = std::function<void()>;

        EventLoop();
        ~EventLoop();

        /// loop forever
        /// Must be called in the same thread as creation of the object.
        void loop();

        // Quits loop.
        void quit();

        // Time when poll returns, usually means data arrival.
        Timestamp pollReturnTime() const { return pollReturnTime_; }

        /**
         * Runs callback immediately in the loop thread.
         * It wakes up the loop, and run the cb.
         * If in the same loop thread, cb is run within the function.
         * Safe to call from other threads.
         * 
         * 在当前loop中执行cb
         */
        void runInLoop(Functor cb);

        /**
         * Queues callback in the loop thread.
         * Runs after finish pooling.
         * Safe to call from other threads.
         * 
         * 把cb放入队列中，唤醒loop所在的线程，执行cb
         */
        void queueInLoop(Functor cb);


        // internal usage
        void wakeup();      // 用来唤醒loop所在线程
        void updateChannel(Channel* channel);   // Channel -> EventLoop -> Poller
        void removeChannel(Channel* channel);
        bool hasChannel(Channel* channel);

        // 判断EventLoop对象是否在自己的线程里面
        bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
    private:
        void handleRead();          // wakeup
        void doPendingFunctors();

        using ChannelList = std::vector<Channel*>;

        std::atomic_bool looping_;          // 原子操作，通过CAS实现
        std::atomic_bool quit_;             // quit_常用于线程退出的标志。

        const pid_t threadId_;              // 记录当前loop所在的thread id

        Timestamp pollReturnTime_;          // poller返回发生事件的channels的时间点
        std::unique_ptr<Poller> poller_;

        //当mainReactor获取一个新用户的channel时，通过轮询算法选择一个subReactor，通过该成员通知subReactor处理channel。
        int wakeupFd_;
        // we don't expose Channel to client.
        std::unique_ptr<Channel> wakeupChannel_;

        ChannelList activeChannels_;

        std::atomic_bool callingPendingFunctors_;    // 标识当前loop是否有需要执行的回调操作
        std::vector<Functor> pendingFunctors_;      // 存储loop需要执行的所有的回调操作
        std::mutex mutex_;                          // 互斥锁，用来保护上面vector的线程安全
};