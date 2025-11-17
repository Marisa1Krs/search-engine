#include "TcpConnetion.h"
#include"EventLoop.h"
#include<iostream>
TcpConnetion::TcpConnetion()
:sock()
,sockIO()
,localAddr(getLocalAddr())
,peerAddr(getPeerAddr())
,fd(sock.getfd())
{

}
TcpConnetion::TcpConnetion(int fd,EventLoop* loop)
:sock(fd)
,sockIO(fd)
,localAddr(getLocalAddr())
,peerAddr(getPeerAddr())
,evtLoop(loop)
,fd(sock.getfd())
{

}
TcpConnetion::~TcpConnetion()
{

}
InetAddress TcpConnetion::getLocalAddr(){
    struct sockaddr_in temp;
    socklen_t len=sizeof(temp);
    int err=getsockname(sock.getfd(),(struct sockaddr*)&temp,&len);
    if(err==-1){
        perror("getsockname");
    }
    return InetAddress(temp);
}
InetAddress TcpConnetion::getPeerAddr(){
    struct sockaddr_in temp;
    socklen_t len=sizeof(temp);
    int err=getpeername(sock.getfd(),(struct sockaddr*)&temp,&len);
    if(err==-1){
        perror("getsockname");
    }
    return InetAddress(temp);
}
string TcpConnetion::receive(){
    char buf[1024];
    sockIO.readline(buf);
    return string(buf);
}
void TcpConnetion::send(const string &msg){
    sockIO.writen(msg.c_str(),msg.size()+1);
}
string TcpConnetion::toString(){}
void TcpConnetion::setNewConnetCallBack(TcpConnetionCallBack& cb){
        newConnetCallBack=cb;
}
void TcpConnetion::setSendMessgeCallBack(TcpConnetionCallBack& cb){
        sendMessgeCallBack=cb;
}
void TcpConnetion::setCloseConnetCallBack(TcpConnetionCallBack& cb){
        closeConnetCallBack=cb;
}
void TcpConnetion::handleNewConnetCallBack(){
        if(newConnetCallBack){
            newConnetCallBack(shared_from_this());
        }
        else{
            std::cout<<"newConnetCallBack is null"<<std::endl;
        }
}
void TcpConnetion::handleSendMessgeCallBack(){
    if(sendMessgeCallBack){
        sendMessgeCallBack(shared_from_this());
    }
    else{
        std::cout<<"newConnetCallBack is null"<<std::endl;
    }
}
void TcpConnetion::handleCloseConnetCallBack(){
        if(closeConnetCallBack){
            closeConnetCallBack(shared_from_this());
        }
        else{
            std::cout<<"newConnetCallBack is null"<<std::endl;
        }
}
void TcpConnetion::sendInLoop(const string& msg){
    if(evtLoop)
    evtLoop->runInLoop(std::bind(&TcpConnetion::send,this,msg));
}