#include <ymuduo/TcpServer.h>
#include <string>
#include <ymuduo/Logger.h>
#include <ymuduo/EventLoop.h>
#include <functional>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress &addr, const std::string &name) 
    : loop_(loop)
    ,server_(loop, addr, name) {
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));

        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));

        server_.setThreadNum(3);
    }

    void start() {
        server_.start();
    }

    ~EchoServer() {

    }

private:
    // 连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr& conn) {
        if(conn->connected()) {
            LOG_INFO("Connection UP : %s\n", conn->peerAddress().toIpPort().c_str());
        }
        else {
            LOG_INFO("Connection DOWN : %s\n", conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time) {
        std::string msg = buffer->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main() {
    EventLoop loop;
    InetAddress addr(8080, false);
    EchoServer server(&loop, addr, "EchoServer"); // non-blocking  bind 
    server.start();     // listening
    loop.loop();    // 启动 main Reactor

    return 0;
}