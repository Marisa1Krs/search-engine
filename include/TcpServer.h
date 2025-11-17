#ifndef TCPSERVER_H
#define TCPSERVER_H
#pragma once
#include"Acceptor.h"
#include"EventLoop.h"
#include<iostream>
#include<memory>
#include<functional>
using std::string;
class TcpServer
{
    using TcpConnetionPtr=shared_ptr<TcpConnetion>;
    using TcpConnetionCallBack=function<void(const shared_ptr<TcpConnetion>& con)>;
public:
    TcpServer(const string& ip,const string port);
    ~TcpServer();
    void start();
    void stop();
    void setCallBack(TcpConnetionCallBack&& cb1,TcpConnetionCallBack&& cb2,TcpConnetionCallBack&& cb3);
private:
    Acceptor acceptor;
    EventLoop evLoop;
};

#endif