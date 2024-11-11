## noncopyable.h
## Logger.h
1. ... 和 ##__VA_ARGS__ 用法
2. 通过定义宏MUDEBUG来控制LOG_DEBUG宏的行为。
    使用方式:`gcc -DMUDEBUG -o my_program my_program.c`就可以打开调试日志
## Timestamp.h
 1. explicit关键字的使用
## copyable.h
 1. EBO(empty base class optimization)使得派生类不因为继承了一个空基类而增加对象的大小。
## InetAddress.h 
 1. 类型强转`static_cast<target type>(reinterpret_cast<void *>)`
## InetAddress.cc
 1. ip, port 涉及的函数不熟悉
## Channel.h
 1. 前置声明和头文件的区别
 2. 成员变量index_?
 3. 成员变量tie_? 
 4. `std::move()` 将普通变量转换成右值以调用移动拷贝构造/赋值运算符
 5. `void tie(const std::shared_ptr<void>&);` ?
 6. `void set_revents(int revt)`?
     `PollPoller::fillActiveChannels`这个在拷贝发生事件的结构体struct pollfd的revents到channel的revents。相当于结构体 -> 类
## Channel.cc

## Poller.h
 1. `static Poller* newDefaultPoller(EventLoop* loop);`专门文件的妙处: 基类定义文件不要包含派生类的头文件
## Poller.cc

## DefaultPoller.cc

## EPollPoller.h
## EPollPoller.cc
1. namespace {} 匿名命名空间(anonymous namespace)，将里面的变量和函数的可见性限制在该文件里面，避免命名冲突。
2. `Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)`中的扩容操作，select/poll中无需根据当前发生事件的socket数量进行扩容操作:
   1. select: select通过传入的fd_set来监控多个文件描述符的状态，并返回准备好的文件描述符数量。因此，select的返回值直接告知了当前有多少个文件描述符可以读、写或有异常。不过select本身并不维持一个内部的监听集合，而是依赖调用者每次传入的集合，因此调用者始终知道监听的文件描述符数量。
   2. poll: poll也是通过传入的pollfd数组来监控文件描述符的状态。调用者在调用poll时必须提供一个pollfd数组，并指定数组大小。poll的返回值表示当前有事件的文件描述符数量，数组中每个pollfd的revents字段会标明相应的事件。由于poll调用者也明确指定了文件描述符的数量，因此调用者也始终清楚监听的文件描述符数量。
   3. epoll: epoll的不同之处在于，它将监听的文件描述符集合存储在内核空间(即epoll实例)中，而不依赖每次调用传入的文件描述符集合(~~避免频繁的数据拷贝和遍历~~)。因此，当调用epoll_wait时，无法直接知道当前被监听的文件描述符总数。我们只能得知此次调用中有多少个文件描述符发生了事件，但不知道epoll实例中实际注册的总文件描述符数量(~~失去了对集合内具体状态的细粒度控制~~)。
   4. 因此，在epoll中，返回事件数量等于当前容器大小时进行扩容是一种通用的应对方式，防止潜在的事件丢失。
3. `EPollPoller::EPollPoller(EventLoop* loop)`中epollfd_ < 0出错，让LOG_FATAL添加exit(-1)? muduo源码怎么做的？
4. __FUNCTION__: 编译器提供的宏
5. int savedErrno = errno; 防止处理errno时errno被其他线程修改
## CurrentThread.h
1. __thread 线程局部变量
2. __builtin_expect ? 
3. inline只在当前文件起作用，所以函数实现可以放在头文件里面
## CurrentThread.cc
## EventLoop.h
 1. wakeup以及pendingFunctors_？
 2. std::atomic_bool quit_;     // 原子操作，通过CAS实现 什么意思
     1. quit_一个使用原子操作管理的布尔类型。所有操作都是线程安全的，无需加锁。通常用于多线程编程中，线程终止的标志。
     2. CAS(Compare-And-Swap): 是硬件指令? 它是一种原子操作，接受三个参数：**变量当前的值，期望值，新值**。它会将变量的当前值与期望值进行比较，如果相等，就将变量赋成新值，否则保持不变。
     3. quit_常用于线程推出的标志。例如，主线程可以设置quit_为true，而其他工作线程可以通过检查quit_的值来决定是否继续执行还是终止，从而实现线程的安全退出。
3. wakeupFd_: 
   1. 当mainReactor获取一个新用户的channel时，通过轮询算法选择一个subReactor，通过该成员通知subReactor处理channel。eventfd比pipe更高效？
        ```cpp
        #include <sys/eventfd.h>
        int eventfd(unsigned int initval, int flags);
        ```
        > NOTES(man eventfd)
        > Applications can use an eventfd file descriptor instead of a pipe (see pipe(2)) in all cases where a pipe is used simply to signal events. The kernel overhead of an eventfd file descriptor is much lower than that of a pipe, and only one file descriptor is required (versus the two required for a pipe).
        - [What is the difference between eventfd and pipe IPC mechanism in Linux?](https://qr.ae/p2ltt2)
        - [Linux 新的事件等待/响应机制eventfd](https://blog.csdn.net/weixin_36750623/article/details/84522824)
        - [让事件飞 ——Linux eventfd 原理与实践](https://www.cnblogs.com/lihaodonglala/p/11518956.html)

    2. runInLoop和queueInLoop以及wakeup()好奇怪: 
       1. (GPT)Purpose of queueInLoop and wakeup: If runInLoop is called from another thread, it pushes the task into pendingFunctors_ (a task queue) using queueInLoop. This ensures thread safety by locking pendingFunctors_ with MutexLockGuard. Then, wakeup() is called to notify the EventLoop that there are pending tasks to handle, allowing the EventLoop to respond even if it’s blocked in the polling phase.
## EventLoop.cc
1. EventLoop::loop中doPendingFunctors();似乎是在执行其他线程让它做的工作？ 例如？ 
   1. 在`void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)`中将新连接conn派发给ioLoop时，会让ioLoop执行`ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));`，注意conn初始化时已经归属于ioLoop管理，然后ioLoop会执行`queueInLoop(std::move(cb));`最终触发`wakeup();`唤醒ioLoop所在线程起来将conn加入自己管理的poller中。
   
2. 谁在调用`EventLoop::runInLoop`? `void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)`会调用，同时它也可能会调用其他线程的EventLoop
3. lock_guard和unique_lock区别: 前者适用于简单的作用域加/解锁场景，后者更加灵活，可以延迟锁定、手动加/解锁，可以适用于条件变量的场景。
## Thread.h
## Thread.cc
1. pthread_detach分离线程(资源自动回收)，与pthread_join工作线程相区别
   1. thread.join(): Blocks the current thread until the thread identified by *this finishes its execution.
   2. thread.detach(): Separates the thread of execution from the thread object, allowing execution to continue independently. Any allocated resources will be freed once the thread exits.
   3. Thread::start()中的引用捕获`[&]`:
      1. 引用捕获的变量必须在lambda使用期间保持有效。否则，可能会导致悬空引用问题
      2. 在lambda内部对这些变量的修改会直接影响外部变量的值。
## EventLoopThread.h
## EventLoopThread.cc
## EventLoopThreadPool.h
## EventLoopThreadPool.cc

## TcpServer.h
## TcpServer.cc