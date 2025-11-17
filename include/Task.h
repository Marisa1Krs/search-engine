#ifndef TASK_H
#define TASK_H

#pragma once

#include<functional>
using std::function;
class Task
{
public:
    Task();
    function<void()> doTask=nullptr;
    ~Task();
    void setTask(function<void()>&& cb){
        doTask=cb;
    }
private:

};

#endif