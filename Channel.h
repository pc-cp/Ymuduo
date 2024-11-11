#pragma once

#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>
/**
 * 理清楚 EventLoop、channel、poller的关系 
 * Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */
class EventLoop;

class Channel : noncopyable {
    public:
        using EventCallback = std::function<void ()>;
        using ReadEventCallback = std::function<void (Timestamp)>;

        Channel(EventLoop *loop, int fd);
        ~Channel();

        // fd得到poller通知以后，处理事件的
        void handleEvent(Timestamp receiveTime);

        // 设置回调函数对象
        void setReadCallback(ReadEventCallback cb) {
            readCallback_ = std::move(cb);
        }

        void setWriteCallback(EventCallback cb) {
            writeCallback_ = std::move(cb);
        }
        void setCloseCallback(EventCallback cb) {
            closeCallback_ = std::move(cb);
        }
        void setErrorCallback(EventCallback cb) {
            errorCallback_ = std::move(cb);
        }

        // 防止当channel被手动remove掉，channel还在指向回调操作
        void tie(const std::shared_ptr<void>&);

        int fd() const { return fd_; }
        int events() const { return events_; }
        void set_revents(int revt) {revents_ = revt; } // used by poller
        bool isNonEvent() const { return events_ == kNonEvent; }

        void enableReading() { events_ |= kReadEvent; update(); }
        void disableReading() { events_ &= ~kReadEvent; update(); }
        void enableWriting() { events_ |= kWriteEvent; update(); }
        void disableWriting() { events_ &= ~kWriteEvent; update(); }
        void disableAll() {events_ = kNonEvent; update(); }
        bool isWriting() const { return events_ & kWriteEvent; }
        bool isReading() const { return events_ & kReadEvent; }

        // for poller
        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        // one loop per thread
        EventLoop* ownerLoop() { return loop_; }
        // 让eventLoop删除这个channel
        void remove();

    private:

        void update();

        // handleEvent的helper
        void handleEventWithGuard(Timestamp receiveTime);

        static const int kNonEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        EventLoop *loop_;   // 事件循环
        const int fd_;      // fd，poller监听的对象
        int events_;        // 注册感兴趣的事件
        int revents_;       // poller当前监听到的发生事件
        int index_;     // Used by poller

        std::weak_ptr<void> tie_; 
        bool tied_;
        /**
         * 因为channel里面能够获知fd最终发生的具体事件revents,
         * 所以它负责调用具体事件的回调操作
         */
        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
        
};