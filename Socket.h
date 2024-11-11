#pragma once 

#include "noncopyable.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

class InetAddress;

/**
 * Wrapper of socket file descriptor.
 */

class Socket : noncopyable {

    public:
        explicit Socket(int sockfd)
        : sockfd_(sockfd) {

        }

        ~Socket();

        int fd() const { return sockfd_; }

        void bindAddress(const InetAddress& localaddr);
        void listen();

        /**
         * 如果成功，返回一个新创建的文件描述符用于和新用户的连接，并且
         * 其被设置为non-blocking 和 close-on-exec. 
         * *peeraddr被设置。
         * 如果出差，返回-1
         * 
         * // 该函数肯定是在监听描述符处理读事件中调用
         */
        int accept(InetAddress* peeraddr);

        void shutdownWrite();

        void setTcpNoDelay(bool one);
        void setReuseAddr(bool on);
        void setReusePort(bool on);
        void setKeepAlive(bool on);

    private:
        const int sockfd_;
};