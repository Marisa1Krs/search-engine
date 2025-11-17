#include "threadpool.h"
#include"Task.h"
#include<iostream>
threadpool::threadpool(size_t threadnum,size_t queuesize)
:TaskQueue(queuesize)
,threadNum(threadnum)
,is_exit(false)
,queSize(queuesize)
,threads()
{

}

threadpool::~threadpool(){

}
//线程池的开
void threadpool::start(){
    for(int i=0;i<threadNum;i++){
        thread temp(&threadpool::doTask,this);
        threads.push_back(std::move(temp));
    }
}
//线程池的关
void threadpool::stop(){
    while(TaskQueue.isEmpty()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }//每一秒检测一次
    //主线程可能走的比其他线程快，如果主线程先一步走完，剩下任务队列里面的任务不会执行
    is_exit=1;
    TaskQueue.wakeUp();
    for(auto &t:threads){
        t.join();
    }
}
void threadpool::addTask(Task* ptask){

    TaskQueue.push(ptask);

}
Task* threadpool::getTask(){
    return TaskQueue.pop();
}

void threadpool::doTask(){
    while(!is_exit){
        Task* currentTask=getTask();
        if(currentTask){
        currentTask->doTask();
        }
        else{
            cout<<"this is a null task"<<endl;
        }
    }
}