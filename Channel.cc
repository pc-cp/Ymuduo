#include "Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNonEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd) 
  : loop_(loop)
  , fd_(fd)
  , events_(0)
  , revents_(0)
  , index_(-1)
  , tied_(false) {

  }

Channel::~Channel() {
}

// 防止当channel被手动remove掉，channel还在指向回调操作
void Channel::tie(const std::shared_ptr<void>& obj) {
    // 观察者，weak_ptr观察shared_ptr是否为空？
    tie_ = obj;
    tied_ = true;
}

// 让eventLoop删除这个channel，与update同理，也是让poller更改fd相应的事件epoll_ctl
void Channel::remove() {
    loop_->removeChannel(this);
}

/**
 * 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * EventLoop => ChannelList、Poller
 */
void Channel::update() {
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    if(tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard) {
            handleEventWithGuard(receiveTime);
        }
    }
    else {
        handleEventWithGuard(receiveTime);
    }
}

// handleEvent的helper
// 根据poller反馈的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    
    LOG_INFO("channel handleEvent revents: %d\n", revents_);
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if(closeCallback_) {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR) {
        if(errorCallback_) {
            errorCallback_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI)) {
        if(readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT) {
        if(writeCallback_) {
            writeCallback_();
        }
    }
}