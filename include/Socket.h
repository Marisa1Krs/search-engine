#ifndef SOCKET_H
#define SOCKET_H
#include<allfun_of_linux.h>


class Socket
{
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();
    int getfd();
private:
    int fd;
};

#endif