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
    LOG_INFO("%s",msg.c_str());
    json msgJson;
    if(msg[0]=='/')
    msgJson=PageLibPreprocessor::getPtr()->find(msg.substr(1,msg.size()-1));
    else
    msgJson=DictProducer::getPtr()->find(msg,5);
    con->sendInLoop(msgJson.dump()+"\n");
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
    // 配置文件路径（唯一保留的硬编码路径，作为配置入口）
    string confPath="/home/marisa/code1/search-engine/config/serch.conf";
    Configer con(confPath);
    auto& cfg = con.getConfigMap();

    // 初始化日志：路径、缓冲区大小、日志级别
    string logPath = cfg["logPath"];
    int logBufSize = std::stoi(cfg["logBufferSize"]);
    int logLevel = std::stoi(cfg["logLevel"]);
    mylog::init(logPath, logBufSize, static_cast<LogLevel>(logLevel));

    // DictProducer 初始化中文/英文词典（词库构建）
    DictProducer::init(con);
    DictProducer::getPtr()->buildEnDict();
    DictProducer::getPtr()->buildCnDict();

    // 构建倒排索引
    PageLibPreprocessor::init(con);
    PageLibPreprocessor::getPtr()->doProcess();

    // 启动 TCP 服务器
    string serverIp = cfg["serverIp"];
    string serverPort = cfg["serverPort"];
    server = new TcpServer(serverIp, serverPort);

    // 线程池
    int threadCount = std::stoi(cfg["threadPoolThreadCount"]);
    int queueSize = std::stoi(cfg["threadPoolQueueSize"]);
    tpool = new threadpool(threadCount, queueSize);

    server->setCallBack(onNewConnet, onMessage, onClose);
    tpool->start();
    server->start();
    return 0;
}