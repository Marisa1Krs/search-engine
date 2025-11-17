#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#pragma once

#include<queue>
#include<condition_variable>
#include<mutex>
using std::queue;
using std::condition_variable;
using std::mutex;
using std::unique_lock;
class Task;
using Elemtype=Task *;
class taskQueue
{
public:
    taskQueue(int cap);
    ~taskQueue();
    bool isEmpty();
    bool isFull();
    void push(Elemtype elem);
    Elemtype pop();
    //因为pop可能在某个线程里面走的比主线程快，有可能提前一步进入睡觉，导致无法唤醒，所以加入了wakeUp函数避免发生这种情况
    void wakeUp();

private:
    queue<Elemtype> que;
    condition_variable notEmpty;
    condition_variable notFull;
    bool wakeUpFlag;
    mutex mx;
    int captain;
    int size;
};

#endif