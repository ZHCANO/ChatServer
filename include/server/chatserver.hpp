#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

/* 基于 muduo 网络库开发服务器程序
1. 组合 TcpServer 对象
2. 创建 EventLoop 时间循环对象的指针
3. 明确 TcpServer 构造函数需要什么参数，输出 ChatServer 的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo 库会自己分配 I/O 线程和 worker 线程
g++ muduo_server.cpp -o server -lmuduo_net -lmuduo_base -lpthread
g++ muduo_server.cpp -o server -l/usr/include -L/usr/lib -lmuduo_net -lmuduo_base -lpthread
-lmuduo_net -lmuduo_base -lpthread 需要链接的库
*/
class ChatServer {
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop, // 事件循环
            const InetAddress& listenAddr, // IP+Port
            const string& nameArg); // 服务器的名字

    // 启动服务，开启事件循环
    void start();
private:
    // 专门处理用户的连接和断开的回调函数
    void onConnection(const TcpConnectionPtr& conn);

    // 专门处理用户读写事件的回调函数：连接、缓冲区、时间戳
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time);

    TcpServer _server; // 组合的muduo库，实现服务器功能的类对象
    EventLoop* _loop; // 指向事件循环对象的指针
};

#endif