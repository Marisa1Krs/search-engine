#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<thread>
#include<allfun_of_c++.h>
#include<vector>
#include"taskQueue.h"
#pragma once
using std::vector;
using std::thread;
using std::cout;
using std::endl;
class threadpool
{
public:
    threadpool(size_t threadnum,size_t queuesize);
    ~threadpool();
    //线程池的开
    void start();
    //线程池的关
    void stop();
    void addTask(Task* ptask);
    Task* getTask();
    void doTask();//线程池执行的主循环
private:
    size_t threadNum;
    vector<thread> threads;
    size_t queSize;
    bool is_exit;
    taskQueue TaskQueue;
};

#endif