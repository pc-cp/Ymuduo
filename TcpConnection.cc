#include "TcpConnection.h"
#include "Channel.h"
#include "assert.h"
#include "Logger.h"
#include <unistd.h>
#include "EventLoop.h"

void defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG_INFO("%s -> %s is %s\n", conn->localAddress().toIpPort().c_str(), conn->peerAddress().toIpPort().c_str(), (conn->connected() ? "UP" : "DOWN"));
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime) {
    buffer->retrieveAll();
}

static EventLoop* checkLoopNotNull(EventLoop *loop) {
    if(loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
: loop_(checkLoopNotNull(loop))
, name_(nameArg)
, state_(kConnecting)
, reading_(true)            // ?
, socket_(new Socket(sockfd))
, channel_(new Channel(loop, sockfd))
, localAddr_(localAddr)
, peerAddr_(peerAddr) 
, highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), socket_->fd());
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d\n", name_.c_str(), socket_->fd());
    assert(state_ == kDisconnected);
}

void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0) { // peer close 
        handleClose();
    }
    else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead\n");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if(channel_->isWriting()) {
        int savedErrno = 0;
        // 将缓冲区中的数据写入到fd
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if(n > 0) {
            // outputBuffer_.retrieve(n);   // in writeFd
            if(outputBuffer_.readableBytes() == 0) {    // 缓冲区中的数据被全部写入到fd
                channel_->disableWriting();             // 及时关闭写事件，让poll不再响应写事件
                if(writeCompleteCallback_) {
                    // 这里可以用runInLoop?
                    // 个人认为这里肯定不是为了让!isInLoopThread()成立，
                    // 而是这个writeCompleteCallback_不是当前要紧事，所以放在pendingFunctors_，反正每次loop.loop都是会执行的
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if(state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        }
        else {
            LOG_ERROR("TcpConnection::handleWrite.\n");
        }
    }
    else {
        LOG_ERROR("Connection fd = %d is down, no more writing\n", channel_->fd());
    }
}

void TcpConnection::handleClose() {
    LOG_ERROR("fd = %d, state = %d\n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis); // 执行连接关闭的回调
    closeCallback_(guardThis);      // 关闭连接的回调
}

void TcpConnection::handleError() {
    auto getSocketError = [&](int sockfd) -> int {
        int optval;
        socklen_t optlen = static_cast<socklen_t>(sizeof optval);

        if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
            return errno;    
        }
        else {
            return optval;
        }};

    int err = getSocketError(channel_->fd());

    LOG_ERROR("TcpConnection::handleError [%s] - SO_ERROR = %d %s\n", name_.c_str(), err, strerror_tl(err));
}

void TcpConnection::send(std::string& buf) {
    if(state_ == kConnected) {
        if(loop_->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        }
        else {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

/**
 * 发送数据，应用写的快，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void* message, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    
    if(state_ == kDisconnected) {
        LOG_INFO("disconnected, give up writing\n");
        return ;
    }

    // if no thing in output queue, try writing directly
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message, len);

        if(nwrote >= 0) {
            remaining = len - nwrote;

            // 要求发送的数据已经全部完成，就不会再设置EPOLL_OUT
            if(remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else {
            // nwrote < 0
            nwrote = 0;
            if(errno != EWOULDBLOCK/*&& errno != EAGAIN*/) {
                LOG_ERROR("TcpConnection::sendInLoop\n");
                if(errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    // 如果该发送的数据没有全部发送完，剩余的数据需要保存到缓冲区中，
    // 然后给channel注册EPOLL_OUT，poller发现tcp的发送缓冲区有空间，会通知相应的channel继续发送
    if(!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_
        && oldLen < highWaterMark_
        && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(message)+nwrote, remaining);
        if(!channel_->isWriting()) {
            channel_->enableWriting();      // 设置EPOLL_OUT以触发TcpConnection::handleWrite继续处理缓冲区数据一直到发送完毕
        }
    }
}

void TcpConnection::shutdown() {
    if(state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }

}

void TcpConnection::shutdownInLoop() {
    if(!channel_->isWriting()) {        // 说明outputBuffer的数据已经全部发送完
        // we are not writing
        socket_->shutdownWrite();   // 关闭写端会触发EPOLLHUP?
    }
}


// called when TcpServer accepts a new connection
void TcpConnection::connectEstablished() {
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());      //?? 用一个强指针shared_ptr指向channel对应的conn，保证conn存活。
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

// called when TcpServer has removed me from its map
void TcpConnection::connectDestroyed() {
    if(state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}