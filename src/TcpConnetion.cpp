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
    char buf[65536] = {0};
    sockIO.readline(buf);
    string result(buf);
    // 去掉末尾的 '\n'（如果存在）
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}
string TcpConnetion::recvHttp() {
    string result;
    char buf[65536];
    int total = 0;

    while (total < 65536 - 1) {
        // 预览数据
        int peek = ::recv(fd, buf + total, 65536 - 1 - total, MSG_PEEK);
        if (peek <= 0) break;

        // 检查预览数据中是否有 \r\n\r\n
        bool foundEnd = false;
        for (int i = 0; i < peek; ++i) {
            if (i + 3 < peek &&
                buf[total + i] == '\r' &&
                buf[total + i + 1] == '\n' &&
                buf[total + i + 2] == '\r' &&
                buf[total + i + 3] == '\n') {
                // 找到头部结束符，读取到该位置
                int toRead = total + i + 4;
                char readBuf[65536];
                int n = ::recv(fd, readBuf, toRead, 0);
                if (n > 0) {
                    result.append(readBuf, n);
                }
                foundEnd = true;
                break;
            }
        }
        if (foundEnd) break;

        // 未找到，读取全部预览数据
        int n = ::recv(fd, buf + total, peek, 0);
        if (n <= 0) break;
        total += n;
    }

    return result;
}

void TcpConnetion::send(const string &msg){
    sockIO.writen(msg.c_str(), msg.size());
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