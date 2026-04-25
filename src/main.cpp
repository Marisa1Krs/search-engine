#include "Jieba.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
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
#include "EventLoopGroup.h"
#include "TcpServer.h"
#include "threadpool.h"
#include "Task.h"
TcpServer *server = nullptr;
threadpool *tpool = nullptr;

// 服务器就绪标志：构建倒排索引期间为 false，完成后设为 true
// 在构建索引期间，客户端连接将被拒绝并收到提示
volatile bool g_serverReady = false;

// 缓存的静态 HTML 页面内容（启动时从文件加载）
string g_staticHtmlContent;

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
// ============================================================
// HTTP 协议辅助函数
// ============================================================
// 简易 URL 解码：将 %XX 替换为字符
string urlDecode(const string& input) {
    string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '%' && i + 2 < input.size()) {
            char hex[3] = {input[i+1], input[i+2], '\0'};
            char* end;
            long code = strtol(hex, &end, 16);
            if (*end == '\0') {
                result += static_cast<char>(code);
                i += 2;
                continue;
            }
        } else if (input[i] == '+') {
            result += ' ';
            continue;
        }
        result += input[i];
    }
    return result;
}

// 解析 HTTP GET 请求，提取路径和查询参数 q
// 输入格式: "GET /search?q=keyword HTTP/1.1\r\n..."
void parseHttpGet(const string& request, string& path, string& query) {
    // 取请求行（第一行）
    size_t lineEnd = request.find("\r\n");
    if (lineEnd == string::npos) {
        path = "/"; query = "";
        return;
    }
    string reqLine = request.substr(0, lineEnd);

    // 解析 "GET /path?q=xxx HTTP/1.1"
    size_t firstSpace = reqLine.find(' ');
    if (firstSpace == string::npos) { path = "/"; query = ""; return; }
    size_t secondSpace = reqLine.find(' ', firstSpace + 1);
    if (secondSpace == string::npos) { path = "/"; query = ""; return; }

    string uri = reqLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    // 分离路径和查询字符串
    size_t qmark = uri.find('?');
    if (qmark != string::npos) {
        path = uri.substr(0, qmark);
        string queryStr = uri.substr(qmark + 1);
        // 查找 q= 参数
        size_t pos = 0;
        while (pos < queryStr.size()) {
            size_t amp = queryStr.find('&', pos);
            string pair = queryStr.substr(pos, amp == string::npos ? string::npos : amp - pos);
            size_t eq = pair.find('=');
            if (eq != string::npos) {
                string key = pair.substr(0, eq);
                string val = pair.substr(eq + 1);
                if (key == "q") {
                    query = urlDecode(val);
                    break;
                }
            }
            if (amp == string::npos) break;
            pos = amp + 1;
        }
    } else {
        path = uri;
    }
}

// 构建 HTTP 响应字符串
string buildHttpResponse(const json& body, int statusCode = 200, const string& statusText = "OK") {
    string bodyStr = body.dump();
    string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
                      "Content-Type: application/json; charset=utf-8\r\n"
                      "Content-Length: " + std::to_string(bodyStr.size()) + "\r\n"
                      "Connection: keep-alive\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" +
                      bodyStr;
    return response;
}

// 构建 HTTP 响应（支持任意 Content-Type，用于返回 HTML 等非 JSON 内容）
string buildHttpResponse(const string& body, const string& contentType, int statusCode, const string& statusText) {
    string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n"
                      "Content-Type: " + contentType + "\r\n"
                      "Content-Length: " + std::to_string(body.size()) + "\r\n"
                      "Connection: keep-alive\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n" +
                      body;
    return response;
}

// ============================================================
// HTTP 请求处理函数：根据路径路由到不同的搜索服务
// ============================================================
void handleHttpRequest(const string& path, const string& query, shared_ptr<TcpConnetion> con)
{
    LOG_INFO("HTTP path=%s, query=%s", path.c_str(), query.c_str());
    json msgJson;

    if (path == "/search") {
        // 网页搜索（PageLibPreprocessor 倒排索引检索）
        if (!query.empty())
            msgJson = PageLibPreprocessor::getPtr()->find(query);
        else
            msgJson["error"] = "请提供搜索关键词，例如 GET /search?q=关键词";
    } else if (path == "/suggest") {
        // 词典搜索 / 关键词联想（DictProducer 前缀匹配）
        if (!query.empty())
            msgJson = DictProducer::getPtr()->find(query, 5);
        else
            msgJson["info"] = "请输入关键词获取联想提示，例如 GET /suggest?q=关键词";
    } else {
        // 根路径或未知路径，返回 API 使用帮助
        json endpoints = json::array();
        json ep1, ep2;
        ep1["path"] = "/suggest?q=关键词";
        ep1["description"] = "关键词联想（词典搜索）";
        ep2["path"] = "/search?q=关键词";
        ep2["description"] = "网页内容搜索";
        endpoints.push_back(ep1);
        endpoints.push_back(ep2);
        msgJson["usage"] = "搜索引擎 HTTP API";
        msgJson["endpoints"] = endpoints;
    }

    string httpResp = buildHttpResponse(msgJson);
    con->sendInLoop(httpResp);
}
void onNewConnet(const shared_ptr<TcpConnetion> &con)
{
    cout << "新链接到来 " << con->getFd() << endl;
}
void onMessage(const shared_ptr<TcpConnetion> &con)
{
    // 读取完整 HTTP 请求头（直到 \r\n\r\n）
    string httpReq = con->recvHttp();

    // 解析 HTTP GET 请求，提取路径和查询参数
    string path, query;
    parseHttpGet(httpReq, path, query);

    // 根路径 → 返回静态 HTML 页面（即使索引未就绪也可访问前端界面）
    if (path == "/" || path.empty()) {
        if (!g_staticHtmlContent.empty()) {
            string response = buildHttpResponse(g_staticHtmlContent, "text/html; charset=utf-8", 200, "OK");
            con->sendInLoop(response);
        } else {
            json errJson;
            errJson["error"] = "静态页面未加载，请检查 cli/html/cli.html 文件";
            con->sendInLoop(buildHttpResponse(errJson, 500, "Internal Server Error"));
        }
        return;
    }

    // 服务器正在构建倒排索引，拒绝处理请求
    if (!g_serverReady) {
        json errJson;
        errJson["error"] = "服务器正在初始化倒排索引，请稍后再试";
        errJson["status"] = "initializing";
        con->sendInLoop(buildHttpResponse(errJson, 503, "Service Unavailable"));
        LOG_WARN("拒绝请求：服务器正在初始化倒排索引");
        return;
    }

    // 无查询参数时返回 API 帮助信息
    if (query.empty()) {
        json helpJson;
        json endpoints = json::array();
        json ep1, ep2;
        ep1["path"] = "/suggest?q=关键词";
        ep1["description"] = "关键词联想（词典搜索）";
        ep2["path"] = "/search?q=关键词";
        ep2["description"] = "网页内容搜索";
        endpoints.push_back(ep1);
        endpoints.push_back(ep2);
        helpJson["usage"] = "搜索引擎 HTTP API - 请提供查询参数 q";
        helpJson["endpoints"] = endpoints;
        con->sendInLoop(buildHttpResponse(helpJson));
        return;
    }

    // 提交任务到线程池处理
    Task *t = new Task();
    t->setTask(std::bind(&handleHttpRequest, path, query, con));
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

    // 加载静态 HTML 页面到内存缓存（从配置文件路径推导项目根目录）
    string htmlPath = confPath.substr(0, confPath.rfind('/'));     // 去掉 "serch.conf"
    htmlPath = htmlPath.substr(0, htmlPath.rfind('/'));            // 去掉 "config"
    htmlPath += "/cli/html/cli.html";
    std::ifstream ifs(htmlPath);
    if (ifs) {
        g_staticHtmlContent.assign(
            (std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>()
        );
        LOG_INFO("静态页面已加载: %s (%zu bytes)", htmlPath.c_str(), g_staticHtmlContent.size());
    } else {
        LOG_WARN("静态页面文件不存在: %s，根路径将返回 500 错误", htmlPath.c_str());
    }

    // ============================================================
    // 第一阶段：启动 TCP 服务器和线程池
    // TcpServer::start() 阻塞运行主 EventLoop（accept 连接）
    // 此时 g_serverReady = false，客户端连接会收到"正在初始化"提示
    // ============================================================
    string serverIp = cfg["serverIp"];
    string serverPort = cfg["serverPort"];
    int subLoopCount = std::stoi(cfg["subEventLoopCount"]);

    server = new TcpServer(serverIp, serverPort, subLoopCount);

    int threadCount = std::stoi(cfg["threadPoolThreadCount"]);
    int queueSize = std::stoi(cfg["threadPoolQueueSize"]);
    tpool = new threadpool(threadCount, queueSize);

    server->setCallBack(onNewConnet, onMessage, onClose);
    tpool->start();

    // 在后台线程中启动 TcpServer（mainLoop.loop() 阻塞运行主 EventLoop）
    std::thread serverThread([]() {
        LOG_INFO("TCP 服务器已启动，主 EventLoop 监听新连接（索引构建中...）");
        server->start();
    });

    // ============================================================
    // 第二阶段：构建词典和倒排索引（耗时较长）
    // 在此期间如果客户端连接，onMessage() 会拒绝请求
    // ============================================================
    LOG_INFO("开始构建英文词典...");
    DictProducer::init(con);
    DictProducer::getPtr()->buildEnDict();

    LOG_INFO("开始构建中文词典...");
    DictProducer::getPtr()->buildCnDict();

    LOG_INFO("开始构建倒排索引...");
    PageLibPreprocessor::init(con);
    PageLibPreprocessor::getPtr()->doProcess();

    // ============================================================
    // 第三阶段：索引构建完成，允许处理客户端请求
    // ============================================================
    g_serverReady = true;
    LOG_INFO("倒排索引构建完成，服务器已就绪，开始处理客户端请求");

    // 主线程等待服务器线程结束
    serverThread.join();
    return 0;
}

