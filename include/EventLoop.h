#ifndef EVENTLOOP_H
#define EVENTLOOP_H

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
public:
    using TcpConnetionPtr=shared_ptr<TcpConnetion>;
    using TcpConnetionCallBack=function<void(const shared_ptr<TcpConnetion>& con)>;
    using Functor = function<void()>;
    using AcceptCallback = function<void()>;
    // 子 EventLoop 构造：仅处理 I/O 事件，没有 listenFd
    EventLoop();
    // 主 EventLoop 构造：监听 listenFd 处理新连接
    EventLoop(int listenFd);
    ~EventLoop();
    void loop();
    void unloop();
    void waitEpollFd();
    int createEpollFd();
    void addEpollFd(int fd);
    void delEpollFd(int fd);
    void setNewConnetCallBack(TcpConnetionCallBack cb);
    void setSendMessgeCallBack(TcpConnetionCallBack cb);
    void setCloseConnetCallBack(TcpConnetionCallBack cb);
    // 设置 accept 回调（主 EventLoop 使用，listenFd 就绪时调用）
    void setAcceptCallback(AcceptCallback&& cb);
    void handleMessage(int fd);
    int createEventFd();
    void handleread();
    void wakeup();
    void doPendingFunctors();
    void runInLoop(Functor&& cb);
    // 由主 EventLoop 调用：将一个已接受的客户端 fd 分配给此 EventLoop 管理
    void addNewConnection(int fd);
    void delTcpConnetion(int fd);

private:
    int epoll_fd;
    vector<struct epoll_event> evtList;
    bool isLoop;
    int listenFd;       // -1 表示子 EventLoop（不处理 accept）
    map<int,shared_ptr<TcpConnetion>> connects;
    TcpConnetionCallBack newConnetCallBack;
    TcpConnetionCallBack sendMessgeCallBack;
    TcpConnetionCallBack closeConnetCallBack;
    AcceptCallback acceptCallback;
    int evtFd;
    vector<Functor> Pendings;
    mutex mx;
};

#endif
