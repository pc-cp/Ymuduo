#pragma once 

#include <algorithm>
#include <vector>

#include <string.h>
#include <assert.h>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer {
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        explicit Buffer(size_t initialSize = kInitialSize) 
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend) {

        }

        // ~Buffer();

        size_t readableBytes() const {
            return writerIndex_ - readerIndex_;
        }

        size_t writableBytes() const {
            return buffer_.size() - writerIndex_;
        }

        size_t prependableBytes() const {
            return readerIndex_;
        }

        // 返回缓冲区中可读数据的起始地址
        const char* peek() const {
            return begin() + readerIndex_;
        }

        // 从缓冲区中拿走len个字符(这里只是更新下标)
        void retrieve(size_t len) {
            // 拿走的比缓存的少
            if(len < readableBytes()) {
                readerIndex_ += len;
            }
            else {
                retrieveAll();
            }
        }

        // 拿走缓冲区中的全部数据
        void retrieveAll() {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }

        // 把onMessage函数中参数buffer的数据转成string类型
        std::string retrieveAllAsString() {
            return retrieveAsString(readableBytes());
        }

        std::string retrieveAsString(size_t len) {
            std::string result(peek(), len);
            retrieve(len);  // 更新缓冲区下标
            return result;
        }

        void append(const char* data, size_t len) {
            ensureWritableBytes(len);       // 确保缓冲区能够放下len个字符
            std::copy(data, data+len, beginWrite());    // 开始将[data, len]写入到缓冲区
            hasWritten(len);                // 更新可写缓冲区起始位置
        }

        void append(const void* data, size_t len) {
            append(static_cast<const char*>(data), len);
        }

        void ensureWritableBytes(size_t len) {
            if(writableBytes() < len) {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        char* beginWrite() {
            return begin() + writerIndex_;
        }

        const char* beginWrite() const {
            return begin() + writerIndex_;
        }

        void hasWritten(size_t len) {
            writerIndex_ += len;
        }

        /**
         * Read data directly into buffer.
         * 
         * It may implement with readv(2)
         * @return result of read(2), @c errno is saved
         */
        ssize_t readFd(int fd, int* savedErrno);


        ssize_t writeFd(int fd, int* savedErrno);
    private:
        char* begin() {
            return &*buffer_.begin();
        }

        const char* begin() const {
            return &*buffer_.begin();
        }

        void makeSpace(size_t len) {
            // 当前buffer_不够装下 len 个数据
            if(writableBytes() + prependableBytes() < len + kCheapPrepend) {
                buffer_.resize(writerIndex_ + len);
            }
            else {
                // move readable data to the front, make space inside buffer
                size_t readable = readableBytes();
                std::copy(begin() + readerIndex_,
                          begin() + writerIndex_,
                          begin() + kCheapPrepend);

                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;

            }
        }
        std::vector<char> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;
};