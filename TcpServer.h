#pragma once

#include "noncopyable.h"
#include <functional>
#include <atomic>
#include <unordered_map>
#include "Callbacks.h"
#include "TcpConnection.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;
class InetAddress;
/**
 * Tcp Server, supports single-threaded and thread-pool models.
 * 
 * This is an interface class, so don't expose too much details.
 */
class TcpServer : noncopyable {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;

        enum Option {
            kNoReusePort,
            kReusePort,
        };

        TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option = kNoReusePort);
        ~TcpServer();  

        const std::string& inPort() const { return ipPort_; }
        const std::string& name() const { return name_; }
        EventLoop* getLoop() const { return loop_; }

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
        void setThreadNum(int numThreads);

        // valid after calling start (which call eventlooptreadPool->start)
        std::shared_ptr<EventLoopThreadPool> threadPool() {
            return threadPool_;
        }

        /**
         * Starts the server if it's not listening.
         * 
         * It's harmless to call it multiple times.
         * Thread safe.
         */
        void start();

        void setThreadInitCallback(const ThreadInitCallback& cb) {
            threadInitCallback_ = cb;
        }
        // Set connection callback.
        void setConnectionCallback(const ConnectionCallback& cb) {
            connectionCallback_ = cb;
        }
        // set message callback.
        void setMessageCallback(const MessageCallback& cb) {
            messageCallback_ = cb;
        }
        // set write complete callback.
        void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
            writeCompleteCallback_ = cb;
        }

    private:
        /**
         * Not thread safe, but in loop
         */
        void newConnection(int sockfd, const InetAddress& peerAddr);

        // Thread safe
        void removeConnection(const TcpConnectionPtr& conn);

        // Not thread safe, but in loop 
        void removeConnectionInLoop(const TcpConnectionPtr& conn);

        using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

        EventLoop* loop_;       // the acceptor loop
        const std::string ipPort_;
        const std::string name_;
        std::unique_ptr<Acceptor> acceptor_;        // avoid revealing Acceptor
        std::shared_ptr<EventLoopThreadPool> threadPool_;   // sub Reactors

        ConnectionCallback connectionCallback_;     // 有新连接时的回调
        MessageCallback messageCallback_;           // 有读写消息时的回调
        WriteCompleteCallback writeCompleteCallback_;   // 消息发送完以后的回调

        ThreadInitCallback threadInitCallback_;     // 线程创建后初始化回调

        std::atomic_int32_t started_;                // 原子操作，让Tcp::start()只在第一次被调用时进行初始化

        // always in loop thread
        int nextConnId_;                            // 新连接的编号
        ConnectionMap connections_;         // name -> TcpConnection
        
};