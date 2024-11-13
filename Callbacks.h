#pragma once 

#include "Timestamp.h"

#include <functional>
#include <memory>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// All client visible callbacks go here. 
class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
// using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void (const TcpConnectionPtr&, size_t)>;

// the data has been read to (buf len) that TcpConnection::handleRead do.
using MessageCallback = std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)>;



void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);
