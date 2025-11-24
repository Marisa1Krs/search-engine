#include "Jieba.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "Simhasher.hpp"
#include"json.hpp"
#include"mylog.h"
#include"DictProducer.h"
#include"Configer.h"
#include"PageLibPreprocessor.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Acceptor.h"
#include "SockIO.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "threadpool.h"
#include "Task.h"
TcpServer *server = nullptr;
threadpool *tpool = nullptr;
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace simhash;
using json = nlohmann::json;
string logPath="/home/marisa/code1/search-engine/log/log.txt";
vector<string> splitBySpace(const std::string &input)
{
    vector<string> result;
    string current;

    for (char c : input)
    {
        if (c == ' ')
        {
            // 遇到空格且当前字符串不为空时，添加到结果并重置当前字符串
            if (!current.empty())
            {
                result.push_back(current);
                current.clear();
            }
        }
        else
        {
            // 非空格字符则添加到当前字符串
            current += c;
        }
    }

    // 处理最后一个单词（如果存在）
    if (!current.empty())
    {
        result.push_back(current);
    }

    return result;
}
void msgTask(const shared_ptr<TcpConnetion> &con, string &msg)
{
    
}
void onNewConnet(const shared_ptr<TcpConnetion> &con)
{
    cout << "新链接到来 " << con->getFd() << endl;
}
void onMessage(const shared_ptr<TcpConnetion> &con)
{
    string msg = con->receive();
    Task *t = new Task();
    t->setTask(std::bind(&msgTask, con, msg));
    tpool->addTask(t);
}
void onClose(const shared_ptr<TcpConnetion> &con)
{
    cout << "链接已经关闭" << endl;
}
int main()
{
    mylog::init(logPath,4096,LOG_DEBUG);
    // string cnt="/home/marisa/code1/search-engine/data/yuliao/chinese";
    // DictProducer temp(cnt,SplitTool::getPtr());
    // temp.buildCnDict();
    // sleep(5);
    string confPath="/home/marisa/code1/search-engine/config/serch.conf";
    Configer temp(confPath);
    PageLibPreprocessor pageLib(temp);
    pageLib.doProcess();
    server = new TcpServer("127.0.0.1", "8080");
    server->setCallBack(onNewConnet, onMessage, onClose);
    tpool = new threadpool(4, 4);
    tpool->start();
    server->start();
    return 0;
}