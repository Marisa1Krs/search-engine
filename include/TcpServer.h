#ifndef TCPSERVER_H
#define TCPSERVER_H
#pragma once
#include"Acceptor.h"
#include"EventLoop.h"
#include"EventLoopGroup.h"
#include<iostream>
#include<memory>
#include<functional>
using std::string;
class TcpServer
{
    using TcpConnetionPtr=shared_ptr<TcpConnetion>;
    using TcpConnetionCallBack=function<void(const shared_ptr<TcpConnetion>& con)>;
public:
    TcpServer(const string& ip, const string& port, int subLoopCount = 4);
    ~TcpServer();
    void start();
    void stop();
    void setCallBack(TcpConnetionCallBack&& cb1,TcpConnetionCallBack&& cb2,TcpConnetionCallBack&& cb3);
private:
    Acceptor acceptor;          // 监听器（主 reactor 持有）
    EventLoop mainLoop;         // 主 EventLoop：仅处理 accept
    EventLoopGroup subLoops;    // 子 EventLoop 组：处理 I/O
};

#endif
