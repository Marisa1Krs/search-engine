#include "InetAddress.h"

InetAddress::InetAddress(const string & ip,const string port)
:ip(ip)
,port(port)
{   
    memset(&addr,0,sizeof(addr));
    this->addr.sin_family=AF_INET;
    this->addr.sin_addr.s_addr=inet_addr(ip.c_str());
    this->addr.sin_port=htons(atoi(port.c_str()));
}
InetAddress::InetAddress(const struct sockaddr_in &addr)
{
    this->addr=addr;
}
string InetAddress::getip(){
    return ip;
}
string InetAddress::getport(){
    return port;
}
struct sockaddr_in* InetAddress::getSocketaddr(){
    return &addr;
}
InetAddress::~InetAddress()
{

}