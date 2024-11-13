#include "Socket.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h> // snprintf
#include <unistd.h>
#include "InetAddress.h"
#include "Logger.h"
#include "strings.h"
Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr) {
    int ret = ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in));
    if (ret < 0) {
        LOG_FATAL("bind sockfd: %d fail \n", sockfd_);
    }
}

void Socket::listen() {
    int ret = ::listen(sockfd_, SOMAXCONN);
    if(ret < 0) {
        LOG_FATAL("listen sockfd: %d fail \n", sockfd_);
    }
}   

/**
 * 如果成功，返回一个新创建的文件描述符用于和新用户的连接，并且
 * 其被设置为non-blocking 和 close-on-exec. 
 * *peeraddr被设置。
 * 如果出差，返回-1
 * 
 * // 该函数肯定是在监听描述符处理读事件中调用
 */
int Socket::accept(InetAddress* peeraddr) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);

    socklen_t addrlen = static_cast<socklen_t>(sizeof addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)(&addr), &addrlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
    if(connfd < 0) {
        int savedErrno = errno;
        LOG_ERROR("Socket::accept \n");
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:
            case EPERM:
            case EMFILE:
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                LOG_FATAL("unexpected error of ::accept %d \n", savedErrno);
                break;
            default:
                LOG_FATAL("unknown error of ::accept %d \n", savedErrno);
                break;
        }
    }
    else { // connfd >= 0
        peeraddr->setSockAddr(addr);
    }
    return connfd;
    
}

void Socket::shutdownWrite() {
    if(::shutdown(sockfd_, SHUT_WR) < 0) {
        LOG_ERROR("Socket::shutdownWrite \n");
    }
}

void Socket::setTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}
void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
}

// 如果开启了 Keep-Alive 选项，TCP 协议栈会在连接空闲一段时间后定期发送探测数据包，以检测连接是否仍然活跃。
// 如果探测失败（例如对方没有响应），连接会被认为已断开，从而帮助程序及时发现对方是否已经离线或不可达。
void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}
