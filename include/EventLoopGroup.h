#ifndef EVENTLOOPGROUP_H
#define EVENTLOOPGROUP_H

#include "EventLoop.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

using std::vector;
using std::unique_ptr;
using std::thread;

class EventLoopGroup
{
public:
    EventLoopGroup(int count);
    ~EventLoopGroup();

    // 启动所有子 EventLoop（每个在自己的线程中运行）
    void start();

    // 停止所有子 EventLoop
    void stop();

    // 轮询获取下一个子 EventLoop（round-robin）
    EventLoop* getNextLoop();

    // 为所有子 EventLoop 设置连接回调（传值，内部拷贝到每个子 EventLoop）
    void setCallbacks(
        EventLoop::TcpConnetionCallBack cb1,
        EventLoop::TcpConnetionCallBack cb2,
        EventLoop::TcpConnetionCallBack cb3);

    // 获取子 EventLoop 数量
    int size() const { return count; }

private:
    int count;
    std::atomic<int> next;  // round-robin 轮询索引
    vector<unique_ptr<EventLoop>> loops;
    vector<unique_ptr<thread>> threads;
};

#endif
