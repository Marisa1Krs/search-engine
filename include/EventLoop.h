#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include"Acceptor.h"
#include"TcpConnetion.h"
#include<allfun_of_c++.h>
#include<allfun_of_linux.h>
#include<sys/eventfd.h>
#pragma once
using std::vector;
using std::map;
using std::shared_ptr;
using std::function;
using std::mutex;
class TcpConnetion;
class EventLoop
{
    using TcpConnetionPtr=shared_ptr<TcpConnetion>;
    using TcpConnetionCallBack=function<void(const shared_ptr<TcpConnetion>& con)>;
    using Functor = function<void()>;
public:
    EventLoop(Acceptor& acceptor);
    ~EventLoop();
    void loop();
    void unloop();
    void waitEpollFd();
    void handleNewConnection();
    void handleMessage(int fd);
    int createEpollFd();
    void addEpollFd(int fd);
    void delEpollFd(int fd);
    void setNewConnetCallBack(TcpConnetionCallBack&& cb);
    void setSendMessgeCallBack(TcpConnetionCallBack&& cb);
    void setCloseConnetCallBack(TcpConnetionCallBack&& cb);
    int createEventFd();
    void handleread();
    void wakeup();
    void doPendingFunctors();
    void runInLoop(Functor&& cb);
    void delTcpConnetion(int fd);

private:
    int epoll_fd;
    vector<struct epoll_event> evtList;
    bool isLoop;
    Acceptor acceptor;
    map<int,shared_ptr<TcpConnetion>> connects;
    TcpConnetionCallBack newConnetCallBack;
    TcpConnetionCallBack sendMessgeCallBack;
    TcpConnetionCallBack closeConnetCallBack;
    int evtFd;
    vector<Functor> Pendings;
    mutex mx;
};

#endif