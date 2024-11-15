#pragma once 

#include "noncopyable.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Buffer.h"

#include <memory>
#include <atomic>
class Channel;
class EventLoop;

/**
 * TcpServer -> Acceptor -> connfd -> TcpConnection -> Channel -> Poller resigster
 */
class TcpConnection : noncopyable,
                    public std::enable_shared_from_this<TcpConnection> {
    public:
        TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
        ~TcpConnection();

        EventLoop* getLoop() const { return loop_; }
        const std::string& name() const { return name_; }
        const InetAddress& localAddress() const { return localAddr_; }
        const InetAddress& peerAddress() const { return peerAddr_; }
        bool connected() const { return state_ == kConnected; }
        // bool disconnected() const { return state_ == kDisconnected; }

        // 发送数据
        // void send(const void* message, int len);
        // void send(const std::string& message, int len);
        // void send(Buffer* message);

        void send(std::string& buf);

        void shutdown();
        // void forceClose();
        // void forceCloseWithDelay(double seconds);
        // void setTcpNoDelay(bool on);

        // void startRead();
        // void stopRead();
        // bool isReading() const { return reading_; }

        void setConnectionCallback(const ConnectionCallback& cb) {
            connectionCallback_ = cb;
        }

        void setMessageCallback(const MessageCallback& cb) {
            messageCallback_ = cb;
        }

        void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
            writeCompleteCallback_ = cb;
        }

        void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
            highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark;
        }
        void setCloseCallback(const CloseCallback& cb) {
            closeCallback_ = cb;
        }

        // called when TcpServer accepts a new connection
        void connectEstablished();      // should be called only once 

        // called when TcpServer has removed me from its map
        void connectDestroyed();        // should be called only once         

    private:
        enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        void sendInLoop(const void* message, size_t len);
        void shutdownInLoop();
        void setState(StateE s) { state_ = s;}

        EventLoop* loop_;
        const std::string name_;
        std::atomic_int state_;
        bool reading_;

        // we don't expose those classes to client.
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;
        
        const InetAddress localAddr_;
        const InetAddress peerAddr_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        CloseCallback closeCallback_;
        size_t highWaterMark_;

        Buffer inputBuffer_;
        Buffer outputBuffer_;
};