#include "TcpServer.h"
#include"mylog.h"
#include<iostream>

TcpServer::TcpServer(const string& ip, const string& port, int subLoopCount)
:acceptor(ip, port)
,mainLoop(acceptor.getListenFd())  // 主 EventLoop 监听 listenFd
,subLoops(subLoopCount > 0 ? subLoopCount : 4)
{
}

TcpServer::~TcpServer(){
    stop();
    std::cout<<"TcpServer unloop"<<std::endl;
}

void TcpServer::start(){
    acceptor.ready();

    // 先启动所有子 EventLoop（各自在独立线程中运行）
    subLoops.start();

    // 在主 EventLoop 上注册 accept 回调
    // 当 listenFd 就绪时，accept 新连接并轮询分配给子 EventLoop
    mainLoop.setAcceptCallback([this]() {
        int connFd = acceptor.accept();
        if(connFd < 0){
            perror("accept");
            return;
        }
        // round-robin 选择一个子 EventLoop
        EventLoop* subLoop = subLoops.getNextLoop();
        // 通过子 EventLoop 的 runInLoop 将连接添加到该线程管理
        subLoop->runInLoop([connFd, subLoop]() {
            subLoop->addNewConnection(connFd);
        });
    });

    // 主 EventLoop 阻塞运行（处理 accept 和自身 eventfd 唤醒）
    LOG_INFO("TCP 服务器已启动，主 EventLoop 监听新连接，%d 个子 EventLoop 处理 I/O", subLoops.size());
    mainLoop.loop();
}

void TcpServer::stop(){
    subLoops.stop();
    mainLoop.unloop();
    mainLoop.wakeup();
}

void TcpServer::setCallBack(TcpConnetionCallBack&& cb1,TcpConnetionCallBack&& cb2,TcpConnetionCallBack&& cb3){
    // 所有子 EventLoop 共享同一组回调
    subLoops.setCallbacks(std::move(cb1), std::move(cb2), std::move(cb3));
}
