#include "EventLoop.h"
#include"TcpConnetion.h"
EventLoop::EventLoop(Acceptor& acceptor)
:acceptor(acceptor)
,epoll_fd(createEpollFd())
,isLoop(0)
,evtList(1024)
,newConnetCallBack()
,sendMessgeCallBack()
,closeConnetCallBack()
,evtFd(createEventFd())
,mx()
,Pendings()
{
    addEpollFd(acceptor.getListenFd());//监听listenFd
    addEpollFd(evtFd);//监听evtFd
}
EventLoop::~EventLoop(){
    std::cout<<"eventloop unloop"<<std::endl;
}
void EventLoop::loop(){
    isLoop=1;
    while(isLoop){
        waitEpollFd();
    }
}
void EventLoop::unloop(){
    isLoop=0;
}
void EventLoop::waitEpollFd(){
    int num=epoll_wait(epoll_fd,&*evtList.begin(),1024,-1);//等待3秒，没有报错
    if(num==-1){
        perror("epoll_wait");
    }   
    if(!num){
        std::cout<<"dont have fd need to finish"<<std::endl;
    }
    for(int i=0;i<num;i++){
        if(evtList[i].data.fd==acceptor.getListenFd()){
                handleNewConnection();
        }
        else if(evtList[i].data.fd==evtFd){
            handleread();
            doPendingFunctors();
        }
        else{
            char buf[100];
            if(recv(evtList.at(i).data.fd,buf,1,MSG_PEEK)<=0||buf=="/quit"){
                delEpollFd(evtList.at(i).data.fd);
                close(evtList.at(i).data.fd);
                connects[evtList.at(i).data.fd]->handleCloseConnetCallBack();
                continue;
            }
            handleMessage(evtList.at(i).data.fd);
        }
    }
}
void EventLoop::handleNewConnection(){
    int fd=acceptor.accept();
    if(fd<0){
        perror("accept");
    }
    addEpollFd(fd);
   TcpConnetionPtr con(new TcpConnetion(fd,this));//拷贝构造
    connects[fd]=con;
    con->setNewConnetCallBack(newConnetCallBack);
    con->setSendMessgeCallBack(sendMessgeCallBack);
    con->setCloseConnetCallBack(closeConnetCallBack);
    con->handleNewConnetCallBack();
}
void EventLoop::handleMessage(int fd){
    connects[fd]->handleSendMessgeCallBack();
}
int EventLoop::createEpollFd(){
    int fd=epoll_create(10);
    return fd;
}
void EventLoop::addEpollFd(int fd){
    struct epoll_event temp;
    temp.data.fd=fd;
    temp.events=EPOLLIN;
    int err=epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&temp);
    if(err<0){
        perror("epoll_ctl add fd");
    }
}
void EventLoop::delEpollFd(int fd){
    struct epoll_event temp;
    temp.data.fd=fd;
    temp.events=EPOLLIN;
    int err=epoll_ctl(epoll_fd,EPOLL_CTL_DEL,fd,&temp);
    if(err<0){
        perror("epoll_ctl del fd");
    }
}
void EventLoop::setNewConnetCallBack(TcpConnetionCallBack&& cb){
        newConnetCallBack=std::move(cb);
}
void EventLoop::setSendMessgeCallBack(TcpConnetionCallBack&& cb){
        sendMessgeCallBack=std::move(cb);
}
void EventLoop::setCloseConnetCallBack(TcpConnetionCallBack&& cb){
        closeConnetCallBack=std::move(cb);
}
int EventLoop::createEventFd(){
        int fd=eventfd(0,0);
        return fd;
}
void EventLoop::handleread(){
    uint64_t u = 1;
    int err=read(evtFd,&u,sizeof(uint64_t));
    if(err==-1){
        perror("read evtfd");
    }    
}
void EventLoop::wakeup(){
    uint64_t u = 1;
    ssize_t s = write(evtFd, &u, sizeof(uint64_t));
    if (s != sizeof(uint64_t)) {
        perror("write");
    }
}
void EventLoop::doPendingFunctors(){
    mx.lock();
    vector<Functor> temp;
    temp.swap(Pendings);
    mx.unlock();
    for(auto &cb:temp)cb();
}
void EventLoop::runInLoop(Functor&& cb){
    mx.lock();
    Pendings.push_back(cb);
    mx.unlock();
    wakeup();
}
