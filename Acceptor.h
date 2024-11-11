#pragma once 

#include <functional>
#include "Channel.h"
#include "Socket.h"
#include "noncopyable.h"

class EventLoop;
class InetAddress;

/**
 * 
 * Acceptor of incoming TCP connections.
 */

class Acceptor : noncopyable {
    public:
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
        
        Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback& cb) {
            newConnectionCallback_ = cb;
        }

        void listen();

        bool listening() const { return listening_; }

    private:
        void handleRead();

        EventLoop* loop_;       // Acceptor用的就是用户定义的那个baseLoop，即main Reactor
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listening_;
        int idleFd_;      // fd？
};