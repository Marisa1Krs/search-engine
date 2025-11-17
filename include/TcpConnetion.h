#ifndef TCPCONNETION_H
#define TCPCONNETION_H
#pragma once
#include"SockIO.h"
#include"Acceptor.h"
#include<memory>
#include<functional>
using std::string;
using std::function;
using std::shared_ptr;
class EventLoop;
class TcpConnetion 
:public std::enable_shared_from_this<TcpConnetion>
{
    using TcpConnetionPtr=shared_ptr<TcpConnetion>;
    using TcpConnetionCallBack=function<void(const shared_ptr<TcpConnetion>& con)>;
public:
    TcpConnetion();
    TcpConnetion(int fd,EventLoop* loop);
    ~TcpConnetion();
    InetAddress getLocalAddr();
    InetAddress getPeerAddr();
    string receive();
    void send(const string &msg);
    string toString();
    void setNewConnetCallBack(TcpConnetionCallBack& cb);
    void setSendMessgeCallBack(TcpConnetionCallBack& cb);
    void setCloseConnetCallBack(TcpConnetionCallBack& cb);
    void handleNewConnetCallBack();
    void handleSendMessgeCallBack();
    void handleCloseConnetCallBack();
    void sendInLoop(const string & msg);
    int getFd(){
        return this->fd;
    }
    EventLoop* getEventLoop(){
        return this->evtLoop;
    }
private:
    SockIO sockIO;
    Socket sock;
    InetAddress localAddr;
    InetAddress peerAddr;
    TcpConnetionCallBack newConnetCallBack;
    TcpConnetionCallBack sendMessgeCallBack;
    TcpConnetionCallBack closeConnetCallBack;
    EventLoop* evtLoop;
    int fd;
    
};

#endif