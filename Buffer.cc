#include "Buffer.h"

#include "errno.h"
#include "sys/uio.h"
#include <unistd.h>

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

/**
 * 从fd上读取数据，Poller工作在LT(level-triggered)模式
 * Buffer缓冲区是有大小的，但是从fd上读数据时却不知道其大小
 */
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536] = {0};     // 栈上的buffer 
    struct iovec vec[2];
    const size_t writeable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // When there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1(??64k) bytes at most.
    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0) {
        *savedErrno = errno;
    }
    else if(static_cast<size_t>(n) <= writeable) {
        writerIndex_ += n;      // 更新可写缓冲区下标
    }
    else {
        writerIndex_ = buffer_.size();  
        append(extrabuf, n - writeable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* savedErrno) {
     // 将缓冲区中的数据写入到fd
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n < 0) {
        *savedErrno = errno;
    }
    else if(n == 0) {
        // nothing write to fd
    }
    else {
        retrieve(n);
    }
    return n;
}