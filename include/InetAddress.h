#ifndef INETADDRESS_H
#define INETADDRESS_H
#include<allfun_of_linux.h>
#include<iostream>
using std::string;
class InetAddress
{
public:
    InetAddress(const string & ip,const string port);
    InetAddress(const struct sockaddr_in &addr);
    ~InetAddress();
    string getip();
    string getport();
    struct sockaddr_in* getSocketaddr();
private:
    struct sockaddr_in addr;
    string ip;
    string port;
};

#endif