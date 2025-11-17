#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include<iostream>
#include<allfun_of_linux.h>
#include"Socket.h"
#include"InetAddress.h"
#pragma once

class Acceptor
{
public:
    Acceptor(const string ip,const string port);
    ~Acceptor();
    void ready();
    void setReuseAddr();
    void setReusePort();
    void bind();
    void listen();
    int accept();
    int getListenFd();
private:
    Socket sock;
    InetAddress inetaddr;
};

#endif