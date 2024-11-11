#pragma once 

#include <netinet/in.h>
#include "copyable.h"
#include <string>

class InetAddress : public copyable {
    public:
        // Constructs an endpoint with given port number.
        // Mostly used in TcpServer listening.
        explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");

        // Constructs an endpoint with given ip and port number.
        // explicit InetAddress(std::string& ip, uint16_t port, bool ipv6 = false);

        // Constructs an endpoint with given struct @c sockaddr_in
        // Mostly used when accepting new connections
        explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {
        }

        std::string toIp() const;
        std::string toIpPort() const;
        uint16_t toPort() const;

        // const struct sockaddr* getSockAddr() const { return static_cast<const struct sockaddr*>(reinterpret_cast<const void*>(&addr_)); }
        const sockaddr_in* getSockAddr() const {
            return &addr_;
        }

    private:
        struct sockaddr_in addr_;
            
};