#include "Poller.h"
#include "EPollPoller.h"
// 派生类的头文件

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if(::getenv("MUDUO_USE_POLL")) {
        // TODO
        // return new PollPoller(loop);
        return nullptr;
    }
    else {
        // TODO
        // return new EPollPoller(loop);
        return new EPollPoller(loop);
    }
}