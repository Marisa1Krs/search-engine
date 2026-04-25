#include "EventLoopGroup.h"
#include <iostream>

EventLoopGroup::EventLoopGroup(int cnt)
: count(cnt > 0 ? cnt : 1)
, next(0)
{
    for (int i = 0; i < count; ++i) {
        loops.push_back(unique_ptr<EventLoop>(new EventLoop()));
    }
}

EventLoopGroup::~EventLoopGroup()
{
    stop();
}

void EventLoopGroup::start()
{
    for (int i = 0; i < count; ++i) {
        threads.push_back(unique_ptr<thread>(new thread([this, i]() {
            loops[i]->loop();
        })));
    }
}

void EventLoopGroup::stop()
{
    for (int i = 0; i < count; ++i) {
        if (loops[i]) {
            loops[i]->unloop();
            // wakeup the event loop so it can exit from epoll_wait
            loops[i]->wakeup();
        }
    }
    for (auto& t : threads) {
        if (t && t->joinable()) {
            t->join();
        }
    }
    threads.clear();
}

EventLoop* EventLoopGroup::getNextLoop()
{
    int idx = next.fetch_add(1) % count;
    return loops[idx].get();
}

void EventLoopGroup::setCallbacks(
    EventLoop::TcpConnetionCallBack cb1,
    EventLoop::TcpConnetionCallBack cb2,
    EventLoop::TcpConnetionCallBack cb3)
{
    for (int i = 0; i < count; ++i) {
        // 按值拷贝（不能 move，否则第一次迭代后 cb1/cb2/cb3 变为空）
        loops[i]->setNewConnetCallBack(cb1);
        loops[i]->setSendMessgeCallBack(cb2);
        loops[i]->setCloseConnetCallBack(cb3);
    }
}
