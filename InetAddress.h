#pragma once 

#include <netinet/in.h>
#include "copyable.h"
#include <string>

class InetAddress : public copyable {
    public:
        // Constructs an endpoint with given port number.
        // Mostly used in TcpServer listening.
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