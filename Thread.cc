#include "Thread.h"
#include "CurrentThread.h"
#include "semaphore.h"
#include <assert.h>
std::atomic_int32_t Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string& name) 
: started_(false)
, joined_(false)
, tid_(0)
, func_(std::move(func))
, name_(name) {
    setDefaultName();
}

Thread::~Thread() {
    if(started_ && !joined_) {
        thread_->detach();   // 分离线程
    }
}
 
void Thread::start() {
    started_ = true;

    sem_t sem;
    sem_init(&sem, 0, 0);

    thread_ = std::make_shared<std::thread>([&]() -> void {
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        func_();
    });
    // 这里必须等待获取上面创建新线程的tid值
    sem_wait(&sem);
    assert(tid_ > 0);
}
void Thread::join() {
    joined_ = true;
    thread_->join();
}


void Thread::setDefaultName() {
    int num = ++numCreated_;
    if(name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}


// #include <iostream>

// std::atomic_bool quit_(false);
// void worker() {
//     while(!quit_.load()) {
//         std::this_thread::sleep_for(std::chrono::microseconds(500000)); // 0.5 second
//         std::cout << "Working..." << std::endl;
//     }
//     std::cout << "Thread exiting..." << std::endl;
// }

// int main() {
//     Thread t(worker, "t1");
//     t.start();

//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     quit_.store(true);

//     t.join();
//     return 0;
// }