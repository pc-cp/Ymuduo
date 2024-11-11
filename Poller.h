#pragma once 

#include "noncopyable.h"
#include <vector>
#include <unordered_map>
#include "EventLoop.h"
#include "Timestamp.h"

class Channel;

// muduo库中多路复用分发器的核心IO模块
class Poller : noncopyable {
    public:
        using ChannelList = std::vector<Channel*>;

        Poller(EventLoop* loop);
        virtual ~Poller();

        // 以下三个纯虚函数
        // Polls the IO events.
        // Must be called in the loop thread.
        virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

        // Changes the interedted IO events.
        // Must be called in the loop thread.
        virtual void updateChannel(Channel* channel) = 0;

        // Remove the channel, when it destructs.
        // Must be called in the loop thread.
        virtual void removeChannel(Channel* channel) = 0;

        virtual bool hasChannel(Channel* channel) const;
        // EventLoop可以通过该接口获取默认的IO复用的具体实现(poll? epoll? select?)
        static Poller* newDefaultPoller(EventLoop* loop);

    protected:
        // fd -> Channel*
        using ChannelMap = std::unordered_map<int, Channel*>;
        ChannelMap channels_;
    private:
        EventLoop *ownerLoop_;      // 定义Poller所属事件循环
};