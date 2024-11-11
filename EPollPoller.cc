#include "EPollPoller.h"
#include <sys/epoll.h>
#include "Logger.h"
#include <unistd.h>
#include "Channel.h"
#include <strings.h>
#include <assert.h>
// 匿名空间，仅该文件可访问
namespace {
    // channel 未添加到poller中 
    const int kNew = -1;
    // channel 已添加到poller中
    const int kAdded = 1;
    // channel 从poller中删除
    const int kDeleted = 2;     
} // namespace


EPollPoller::EPollPoller(EventLoop* loop) 
: Poller(loop) 
, epollfd_(::epoll_create1(EPOLL_CLOEXEC))
, events_(kInitEventListSize) {
    if(epollfd_ < 0) {
        LOG_FATAL("EPollPoller::EPollPoller");
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    LOG_INFO("func=%s => fd total count:%ld\n", __FUNCTION__, channels_.size());
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if(numEvents > 0) {
        LOG_INFO("%d events happened \n", numEvents);
        // 有numEvents个事件发生
        fillActiveChannels(numEvents, activeChannels);

        // 扩容
        if(static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
    else if(numEvents == 0) {
        // nothing happened;
#ifdef MUDEBUG
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
#endif
    }
    else {
        // error happens
        /*
            EINTR
            The call was interrupted by a signal handler before either (1) any of the requested events occurred or (2) the timeout  ex‐
            pired; see signal(7).
        */
        if(savedErrno != EINTR) {
            errno = savedErrno;
            LOG_ERROR("EPollPoller::poll()");
        }
    }
    return now;
}

// 将发生事件的epoll_event -> Channel*
void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for(int i = 0; i < numEvents; ++i) {
        // void* -> Channel*
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        // 将fd当前发生的事情放到Channel对象中
        // EventLoop就拿到了它的poller给它返回所有发生事件的channel列表
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

// channel->update/remove -> EventLoop update/remove -> Poller update/remove
void EPollPoller::updateChannel(Channel* channel) {
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    if(index == kNew || index == kDeleted) {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if(index == kNew) {
            channels_[fd] = channel;
        }
        else {  // index == kDeleted
            assert(channels_.find(fd) != channels_.end());
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else { 
        // update existing one with EPOLL_CTL_MOD/DEL (index == kAdded)
        int fd = channel->fd();
        if(channel->isNonEvent()) {
            update(EPOLL_CTL_DEL, channel);
            // 这里仅仅设置channel本身的状态，并没有将其从channels_(fd, channel*)中删除。
            channel->set_index(kDeleted);
        }
        else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel，可能在EPollPoller::updateChannel之后使用
void EPollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    int index = channel->index();
    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, channel->fd());
    assert(index == kAdded || index == kDeleted);
    // 这是真的删除，为啥需要line 98仅仅需要设置channel状态，而这里是真删？
    size_t n = channels_.erase(fd);
    if(index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// updateChannel 和 removeChannel的helper
void EPollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        // set operation fail
        if(operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl op: %d fd: %d\n", errno, fd);
        }
        else {
            LOG_FATAL("epoll_ctl op: %d fd: %d\n", errno, fd);
        }
    }
}