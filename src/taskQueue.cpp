#include "taskQueue.h"
#include<iostream>
taskQueue::taskQueue(int cap)
:notEmpty()
,notFull()
,mx()
,captain(4)
,size(0)
,que()
,wakeUpFlag(1)
{

}
bool taskQueue::isEmpty(){
    return que.size()==0;
}
bool taskQueue::isFull(){
    return que.size()==captain;
}
void taskQueue::push(Elemtype elem){
    
    unique_lock<mutex> umx(mx);
    while(isFull()){
        std::cout<<que.size()<<std::endl;
        notFull.wait(umx);
    }
    que.push(elem);
    notEmpty.notify_one();
}
Elemtype taskQueue::pop(){
    unique_lock<mutex> umx(mx);
    while(isEmpty()&&wakeUpFlag){
        notEmpty.wait(umx);
    }
    Elemtype temp=que.front();
    que.pop();
    umx.unlock();
    return temp;
}
void taskQueue::wakeUp(){
    wakeUpFlag=0;
    notEmpty.notify_all();
    
}
taskQueue::~taskQueue()
{

}