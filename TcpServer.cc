#include "TcpServer.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"
#include "assert.h"
#include "EventLoop.h"
#include <string>
#include <strings.h>

EventLoop* checkLoopNotNull(EventLoop *loop) {
    if(loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option)
: loop_(checkLoopNotNull(loop))
, ipPort_(listenAddr.toIpPort()) 
, name_(nameArg)
, acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
, threadPool_(new EventLoopThreadPool(loop, nameArg))
, connectionCallback_(defaultConnectionCallback)
, messageCallback_(defaultMessageCallback)
, started_(0)
, nextConnId_(1) {

    /** 
     * accept接收到新连接时在handleRead里调用的
     * 该函数TcpServer::newConnection主要是将新的
     * 通信socket派发给subReactor
    */
    
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, _1, _2));
} 

TcpServer::~TcpServer() {
    for(auto& item : connections_) {
        // 这个局部的shared_ptr智能指针对象，出作用域，可以自动释放new出来的TcpConnection对象资源了。
        TcpConnectionPtr conn(item.second);
        
        item.second.reset();        // 不再用shared_ptr指向
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

/**
 * Set the number of threads for handling input.
 * 
 *  Always accepts new connection in loop's thread.
 * Must be called before start (listening in Poll)
 * @param numThreads
 * - 0 means all I/O in loop's thread, no thread will created.
 *      this is the default value.
 * - 1 means all I/O in another thread.
 * - N means a thread pool with N threads, new connections are assigned on a round-robin basis.
 */
void TcpServer::setThreadNum(int numThreads) {
    assert(0 <= numThreads);   
    threadPool_->setThreadNum(numThreads);
}

/**
 * Starts the server if it's not listening.
 * 
 * subReactor先开启loop，然后 mainReactor 才 listening
 * 
 * It's harmless to call it multiple times.
 * Thread safe.
 */
void TcpServer::start() {
    if(started_++ == 0) {       // 防止TcpServer start多次
        threadPool_->start(threadInitCallback_);    // 启动subReactor的loop
        assert(!acceptor_->listening());
        loop_->runInLoop(
            std::bind(&Acceptor::listen, acceptor_.get()));     // main Reactor看见监听
    }
}

/**
 * Not thread safe, but in loop
 * 
 * 当accept监听到新用户时执行
 * 
 * 1. 使用round-robin找到sub Reactor
 * 2. 将当前connfd封装成channel派发给sub Reactor
 * 2. 唤醒sub Reactor
 */
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    struct sockaddr_in localaddr;
    ::bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if(::getsockname(sockfd, (sockaddr*)&localaddr, &addrlen) < 0) {
        LOG_ERROR("sockets::getLocalAddr\n");
    }

    InetAddress localAddr(localaddr);

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;

    // 将暴露给用户的接口绑定，待事件发生时调用。
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    /**
     * 这里主要为了防止单线程模式，如果ioLoop保证不是main Reactor，则完全可以用sub Reactor的queueInLoop
     */
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));

}

// Thread safe
void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

// Not thread safe, but in loop 
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s \n", name_.c_str(), conn->name().c_str());
    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}