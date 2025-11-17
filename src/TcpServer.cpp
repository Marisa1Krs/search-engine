#include "TcpServer.h"
#include<iostream>
TcpServer::TcpServer(const string& ip,const string port)
:acceptor(ip,port)
,evLoop(acceptor)
{
   
}

TcpServer::~TcpServer(){
    std::cout<<"TcpServer unloop"<<std::endl;
}

void TcpServer::start(){
    acceptor.ready();
    evLoop.loop();
}

void TcpServer::stop(){
    evLoop.unloop();
}
void TcpServer::setCallBack(TcpConnetionCallBack&& cb1,TcpConnetionCallBack&& cb2,TcpConnetionCallBack&& cb3){
    if(!cb1){
        std::cout<<"cb1 is null"<<std::endl;
        perror("cb1");
    }
    evLoop.setNewConnetCallBack(std::move(cb1));    
    evLoop.setSendMessgeCallBack(std::move(cb2));
    evLoop.setCloseConnetCallBack(std::move(cb3));
}