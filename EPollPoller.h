#pragma once 

#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

struct epoll_event;
class EPollPoller : public Poller {
    public:
        EPollPoller(EventLoop* loop);
        ~EPollPoller() override;

        // 重写三个纯虚函数
        Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
        void updateChannel(Channel* channel) override;
        void removeChannel(Channel* channel) override;

    private:
        static const int kInitEventListSize = 16;
        using EventList = std::vector<struct epoll_event>;

        // 将发生事件的epoll_event -> Channel*
        void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

        // updateChannel 和 removeChannel的helper
        void update(int operation, Channel* channel);

        int epollfd_;
        EventList events_;
};