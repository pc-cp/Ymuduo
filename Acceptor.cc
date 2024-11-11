#include "Acceptor.h"
#include "InetAddress.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include "Logger.h"
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0) {
        LOG_FATAL("%s:%s:%d listen socket create err: %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport) 
: loop_(loop)
, acceptSocket_(createNonblocking())
, acceptChannel_(loop, acceptSocket_.fd())
, listening_(false)
, idleFd_(::open("/dev/null", O_RDONLY|O_CLOEXEC)) {

    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen() {
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();     // listening -> Poller
}

// listenfd有事件发生，即有新用户来连接了
void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);   // 轮询 subReactor，将该connfd交给它管理
        }
        else {
            // 有新用户来，但是当前没有能力处理
            ::close(connfd);
        }
    }
    else {
        LOG_ERROR("%s:%s:%d accept handleRead err: %d \n", __FILE__, __FUNCTION__, __LINE__, errno);

        // EMFILE The per-process limit on the number of open file descriptors has been reached.
        if(errno == EMFILE) {
            LOG_FATAL("%s:%s:%d socket reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}

// #include <iostream>
// g++ -o Acceptor Acceptor.cc Logger.cc Socket.cc Channel.cc  InetAddress.cc Timestamp.cc EventLoop.cc Poller.cc CurrentThread.cc EPollPoller.cc DefaultPoller.cc -pthread
// int main() {
//     createNonblocking();
// }