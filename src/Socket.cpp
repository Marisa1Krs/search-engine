#include "Socket.h"

Socket::Socket()
{
    fd=::socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1){
        perror("socket");
    }
}
Socket::Socket(int fd){
    this->fd=fd;
}
int Socket::getfd(){
    return fd;
}
Socket::~Socket()
{
    close(fd);
}