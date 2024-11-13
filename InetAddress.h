#pragma once 

#include <netinet/in.h>
#include "copyable.h"
#include <string>

class InetAddress : public copyable {
    public:
        // Constructs an endpoint with given port number.
        // Mostly used in TcpServer listening.
        /**
         * 如果两个构造函数都可以匹配，但其中一个构造函数不依赖默认参数来完成匹配，编译器会优先选择这个构造函数。
         * 这样可以减少歧义，避免编译器对调用意图的错误推测。
         * 
         * InetAddress addr(8080); 匹配第一个ctor
         */
        explicit InetAddress(uint16_t port, bool loopbackOnly = false);
        explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");

        // Constructs an endpoint with given ip and port number.
        // explicit InetAddress(std::string& ip, uint16_t port, bool ipv6 = false);

        // Constructs an endpoint with given struct @c sockaddr_in
        // Mostly used when accepting new connections
        explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {
        }

        sa_family_t family() const { return addr_.sin_family; }
        std::string toIp() const;
        std::string toIpPort() const;
        uint16_t toPort() const;

        // const sockaddr* getSockAddr() const { 
        //     return reinterpret_cast<const sockaddr*>(&addr_); 
        // }
        // void Socket::bindAddress(const InetAddress& localaddr) 不兼容
        const sockaddr_in* getSockAddr() const {
            return &addr_;
        }

        void setSockAddr(const sockaddr_in& addr) { addr_ = addr; }

    private:
        struct sockaddr_in addr_;
            
};