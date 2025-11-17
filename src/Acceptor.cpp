#include "Acceptor.h"
#include<iostream>
Acceptor::Acceptor(const string ip,const string port)
:inetaddr({ip,port})
,sock()
{
}
void Acceptor::bind(){
    int err=::bind(sock.getfd(),(struct sockaddr*)inetaddr.getSocketaddr(),sizeof(struct sockaddr));
    if(err==-1){
        perror("bind");
        return;
    }
}
void Acceptor::listen(){
    int err=::listen(sock.getfd(),128);
    if(err==-1){
        perror("listen");
        return;
    }
}
int Acceptor::accept(){
    int _fd=::accept(sock.getfd(),nullptr,nullptr);
    return _fd;
}
void Acceptor::setReuseAddr(){
    int optval = 1;
    if (setsockopt(sock.getfd(), SOL_SOCKET, SO_REUSEADDR,&optval, sizeof(optval)) < 0) {
        perror("setsockopt(SO_REUSEPORT) failed");
    }
}
void Acceptor::setReusePort(){
    int optval = 1;
    if (setsockopt(sock.getfd(), SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt(SO_REUSEPORT) failed");
    }
}
void Acceptor::ready(){
    setReuseAddr();
    setReusePort();
    bind();
    listen();
}
int Acceptor::getListenFd(){
    return this->sock.getfd();
}
Acceptor::~Acceptor()
{
    std::cout<<"Acceptor unloop"<<std::endl;
}