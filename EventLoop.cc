#include "EventLoop.h"
#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include <assert.h>

// 防止一个线程创建多个EventLoop
__thread EventLoop* t_loopInThisThread = nullptr;


// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;  // 10 sec

// 用来通知(阻塞的)subReactor起来处理main reactor分发的新连接(channel)
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0) {
        LOG_FATAL("Failed in eventfd: %d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
: looping_(false)
, quit_(false)
, callingPendingFunctors_(false)
, threadId_(CurrentThread::tid())
, poller_(Poller::newDefaultPoller(this))
, wakeupFd_(createEventfd())
, wakeupChannel_(new Channel(this, wakeupFd_)) {
#ifdef MUDEBUG
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
#endif

    if(t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(
        std::bind(&EventLoop::handleRead, this)
    );

    // we are always reading the wakeupfd
    // 每一个eventloop都将监听wakeup channel的EPOLL_IN读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);
    // 监听两类fd:
    // 1. client fd
    // 2. wakeupfd
    while(!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for(Channel* channel: activeChannels_) {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }

        doPendingFunctors(); // main Reactor 分发newConn给 sub Reactor管理
    }

    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true; // EventLoop自己能执行quit? 它不是一直在while(!quit_) ？
    // 其他线程关闭eventLoop很管用
    if(!isInLoopThread()) {
        wakeup();
    }
}

// 
void EventLoop::runInLoop(Functor cb) { 
    if(isInLoopThread()) {  // 在当前的loop线程中执行cb
        cb();
    }                       // main Reactor执行sub Reactor的loop时(cb是mainRector分派给subReactor) 
    else {                  // 在非当前loop线程中执行cb，就需要缓存待执行的cb，待唤醒当前loop线程后执行

        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    /**
     * !isInLoopThread()应对非当前EventLoop线程执行cb
     * callingPendingFunctors_应对是当前EventLoop线程下，但是loop正在执行doPendingFunctors的
     * functors(来自pendingFunctors_)，此时又有新的cb放到了pendingFunctors_，所以需要在functors
     * 执行完后，立即唤醒poll来执行pendingFunctors_中的cb。
     * 
     */
    if(!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    return poller_->hasChannel(channel);
}

// 用来唤醒loop所在的线程，向wakeupFd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒。
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        // subReactor可能无法被唤醒
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8\n", n);
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one) {
        LOG_ERROR("EventLoop:handleRead() reads %ld bytes instead of 8", n);
    }
}


void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor& functor: functors) {
        functor();
    }
    callingPendingFunctors_ = false;
}