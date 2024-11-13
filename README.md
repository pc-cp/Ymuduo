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
   4. `std::atomic_int32_t Thread::numCreated_{0};`的初始化方式，注意赋值运算符。
## EventLoopThread.h: 作用是？
   1. `void start(const ThreadInitCallback& cb = ThreadInitCallback());`中形参使用默认参数的好处？
      - 通过给cb设置默认实参`ThreadInitCallback()`，可以实现更简洁的函数调用，并让调用者选择是否传入回调函数。当没有传入回调时，cb是空的，你可以在函数内部检查它是否为空来决定是否执行回调。
      ```cpp
      void f(const Func& cb = Func()) {
         if(cb) {
            cb();    // 如果cb不为空，则调用它
         }
         else {
                     // cb为空，可以采取其他措施或什么都不做。
         }
      }
      ```
## EventLoopThread.cc: ?
1. EventLoop* EventLoopThread::startLoop()中使用unique_lock，它支持解锁，而lock_guard不支持手动加/解锁。
2. `void EventLoopThread::threadFunc()`这个函数将sub Reactor中栈上对象loop的地址暴露给main Reactor。
   ```cpp
   #include <thread>
   #include <iostream>
   #include <functional>

   using Func = std::function<void()>;

   Func func = Func(); // func is empty

   class Foo {
      public:
         Foo(int* pval = nullptr)  
         : pval_(pval) {
               func = std::bind(&Foo::f, this); // func is not empty
         }

         int* getPointer() const { return pval_; }
         
         void f() {
               std::this_thread::sleep_for(std::chrono::seconds(1)); 

               int val = 100;  // the value in stack of thread
               std::cout << "finish." << std::endl;
               pval_ = &val;
         }

      private:
         int *pval_;
   };


   void worker() {
      while(!func) {
         std::cout << "func is empty" << std::endl;
         std::this_thread::sleep_for(std::chrono::seconds(1)); 
      }

      // 通过将func绑定到Foo的成员函数上，将该线程的数据通过Foo对象传递到其他线程
      func();
   }
   int main() {
      std::thread t(worker);
      std::this_thread::sleep_for(std::chrono::seconds(2)); 

      Foo f;
      std::cout << f.getPointer() << std::endl;

      std::this_thread::sleep_for(std::chrono::seconds(2)); 
      std::cout << f.getPointer() << std::endl;

      t.join();

      return 0;
   }

   /**
      g++ -o main main.cc -pthread
      func is empty
      func is empty
      0
      finish.
      0x7f3d72559cd0
    */ 

   ```
   类似于这里将main函数中的栈上对象f暴露给新创建
## EventLoopThreadPool.h: 看注释
## EventLoopThreadPool.cc: 看注释

## Socket.h
## Socket.cc

## Acceptor.h
## Acceptor.cc
1. `idleFd_(::open("/dev/null", O_RDONLY|O_CLOEXEC))`的作用是防止文件描述符耗尽
2. ```cpp
   if(errno == EMFILE) {
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
   }
   ```

## TcpServer.h
1. `std::atomic_int32_t started_;`作用是？
   1. // 原子操作，让Tcp::start()只在第一次被调用时进行初始化
## TcpServer.cc
1. 在`void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)`中`sockets::getLocalAddr(sockfd)`里调用`::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen)`其返回socket sockfd当前绑定的地址。
   1. 注意sockfd是accept函数返回的通信socket对应的文件描述符。所以也就是该通信socket绑定的是本地地址
   2. > The  accept() system call is used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET).  It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, creates a new connected socket, and  returns  a new file descriptor referring to that socket.  The newly created socket is not in the listening state.  The original socket sockfd is unaffected by this call.
   3. 结合后续和client进行I/O时也是通过该通信sockfd进行的，所以其绑定的是本地地址似乎也是情理之中。
   4. GPT: 即使 accept() 返回的 socket 描述符是一个连接的 socket，这个 socket 仍然与原来监听的 socket 绑定在同一个本地地址上。它仍然使用原来监听 socket 的本地地址和端口进行通信。也就是说，新创建的 socket 的本地地址是监听 socket 的地址，**只是它不再处于监听状态，而是专门用于该连接的通信。**
   5. **这个函数的逻辑说白了就是accept到connfd怎么处理**
## Buffer.h
## Buffer.cc
1. `const ssize_t n = ::readv(fd, vec, iovcnt);`这个函数调用比较新颖，继read、recv之外。它提供了多个额外的栈上缓冲区来接收数据。缓解堆上缓冲区较小的问题。

## TcpConnection.h
1. 公有继承`public enable_shared_from_this<T>`
   1. 公有继承`public enable_shared_from_this<T>`，能够让类T具有成员函数`shared_from_this()`。它能够安全的返回指向当前对象的shared_ptr。而明显的技术"shared_ptr<T>(this)"会造成多个shared_ptr指向同一个对象，但它们的引用计数会失效。
   2. ```cpp
      #include <iostream>
      #include <memory>

      class Foo : public std::enable_shared_from_this<Foo> {
         public:
            std::shared_ptr<Foo> getPtr() {
                  return shared_from_this();   
                  // return std::shared_ptr<Foo>(this); // free(): double free detected in tcache 2
            }
            ~Foo() {
                  std::cout << "Good::~Good() called" << std::endl;
            }
      };

      int main() {
         {
            std::shared_ptr<Foo> sp1(new Foo());
            std::shared_ptr<Foo> sp2 = sp1->getPtr();

            std::cout << sp1.use_count() << std::endl;
            std::cout << sp2.use_count() << std::endl;
         }
      }
      ```
   3. [what-is-the-usefulness-of-enable-shared-from-this](https://stackoverflow.com/questions/712279/what-is-the-usefulness-of-enable-shared-from-this)
   4. [利用std::enable_shared_from_this实现异步调用中的保活机制](https://blog.csdn.net/caoshangpa/article/details/79392878)
## TcpConnection.cc
1. ` TcpConnection::handleWrite()`中` loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));`？
   1. // 这里可以用runInLoop?
      // 个人认为这里肯定不是为了让!isInLoopThread()成立，
      // 而是这个writeCompleteCallback_不是当前要紧事，所以放在pendingFunctors_，反正每次loop.loop都是会执行的
      loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
2. `TcpConnection::connectEstablished()`中`channel_->tie(shared_from_this());`意义何在？？   
   1. 用一个强指针shared_ptr指向channel对应的conn，保证conn存活。
   2. ```cpp
      if(tied_) {
         std::shared_ptr<void> guard = tie_.lock();
         if(guard) {
               handleEventWithGuard(receiveTime);
         }
      }
      else {
         handleEventWithGuard(receiveTime);
      }
      ```
      这里如果conn被remove，那么conn管理的channel将不再执行conn提供的回调。
3. `void TcpConnection::shutdownInLoop()`与EPOLLHUP的关系
