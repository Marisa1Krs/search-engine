#ifndef SOCKIO_H
#define SOCKIO_H
#include<allfun_of_linux.h>
#pragma once

class SockIO
{
public:
    SockIO();
    explicit SockIO(int fd);
    ~SockIO();
    int readn(char *buf,int len);
    int writen(const char *buf,int len);
    int readline(char* buf);
private:
    int fd;
};

#endif