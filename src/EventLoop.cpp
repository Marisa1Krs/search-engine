#include "EventLoop.h"
#include"TcpConnetion.h"

// 子 EventLoop：无 listenFd，仅处理已建立连接的 I/O
EventLoop::EventLoop()
:listenFd(-1)
,epoll_fd(createEpollFd())
,isLoop(false)
,evtList(1024)
,newConnetCallBack()
,sendMessgeCallBack()
,closeConnetCallBack()
,evtFd(createEventFd())
,mx()
,Pendings()
{
    addEpollFd(evtFd);
}

// 主 EventLoop：监听 listenFd，处理新连接
EventLoop::EventLoop(int fd)
:listenFd(fd)
,epoll_fd(createEpollFd())
,isLoop(false)
,evtList(1024)
,newConnetCallBack()
,sendMessgeCallBack()
,closeConnetCallBack()
,evtFd(createEventFd())
,mx()
,Pendings()
{
    addEpollFd(listenFd);
    addEpollFd(evtFd);
}

EventLoop::~EventLoop(){
    std::cout<<"eventloop unloop"<<std::endl;
}

void EventLoop::loop(){
    isLoop=true;
    while(isLoop){
        waitEpollFd();
    }
}

void EventLoop::unloop(){
    isLoop=false;
}

void EventLoop::waitEpollFd(){
    int num=epoll_wait(epoll_fd,&*evtList.begin(),1024,-1);
    if(num==-1){
        perror("epoll_wait");
    }   
    if(!num){
        std::cout<<"dont have fd need to finish"<<std::endl;
    }
    for(int i=0;i<num;i++){
        // 主 EventLoop：listenFd 就绪，调用 accept 回调（由 TcpServer 注册）
        if(listenFd!=-1 && evtList[i].data.fd==listenFd){
            if(acceptCallback){
                acceptCallback();
            }
        }
        else if(evtList[i].data.fd==evtFd){
            handleread();
            doPendingFunctors();
        }
        else{
            char buf[100];
            if(recv(evtList.at(i).data.fd,buf,1,MSG_PEEK)<=0){
                delEpollFd(evtList.at(i).data.fd);
                close(evtList.at(i).data.fd);
                auto it = connects.find(evtList.at(i).data.fd);
                if(it != connects.end()){
                    it->second->handleCloseConnetCallBack();
                }
                continue;
            }
            handleMessage(evtList.at(i).data.fd);
        }
    }
}

void EventLoop::handleMessage(int fd){
    auto it = connects.find(fd);
    if(it != connects.end()){
        it->second->handleSendMessgeCallBack();
    }
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

void EventLoop::setNewConnetCallBack(TcpConnetionCallBack cb){
        newConnetCallBack=std::move(cb);
}

void EventLoop::setSendMessgeCallBack(TcpConnetionCallBack cb){
        sendMessgeCallBack=std::move(cb);
}

void EventLoop::setCloseConnetCallBack(TcpConnetionCallBack cb){
        closeConnetCallBack=std::move(cb);
}

void EventLoop::setAcceptCallback(AcceptCallback&& cb){
        acceptCallback=std::move(cb);
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

void EventLoop::addNewConnection(int fd){
    // 将客户端 fd 加入此 EventLoop 的 epoll 并创建 TcpConnetion
    addEpollFd(fd);
    TcpConnetionPtr con(new TcpConnetion(fd, this));
    connects[fd] = con;
    con->setNewConnetCallBack(newConnetCallBack);
    con->setSendMessgeCallBack(sendMessgeCallBack);
    con->setCloseConnetCallBack(closeConnetCallBack);
    con->handleNewConnetCallBack();
}
