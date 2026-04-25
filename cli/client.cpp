/**
 * @file client.cpp
 * @brief 搜索引擎客户端 - 使用 HTTP 协议连接搜索服务端
 *
 * 用法: ./client <ip> <port>
 * 默认连接: 192.168.159.129:8080
 *
 * API 端点:
 *   GET /suggest?q=关键词  → 词典搜索（关键词联想）
 *   GET /search?q=关键词   → 网页搜索（内容检索）
 *
 * 交互命令:
 *   - 输入关键词直接进行词典搜索（/suggest）
 *   - /search <关键词> 进行网页搜索
 *   - /quit 或 /exit 退出
 *   - /help 显示帮助
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

constexpr int BUFFER_SIZE = 65536;

// ============================================================
// URL 编码：将非 ASCII 和特殊字符转为 %XX 格式
// ============================================================
std::string urlEncode(const std::string& value) {
    std::string result;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += "+";
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            result += buf;
        }
    }
    return result;
}

class SearchClient {
public:
    SearchClient(const std::string& ip, unsigned short port)
        : _ip(ip), _port(port), _sockfd(-1) {}

    ~SearchClient() {
        disconnect();
    }

    bool connect() {
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_sockfd < 0) {
            std::cerr << "[错误] 创建 socket 失败" << std::endl;
            return false;
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_port);

        if (inet_pton(AF_INET, _ip.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "[错误] 无效的 IP 地址: " << _ip << std::endl;
            close(_sockfd);
            _sockfd = -1;
            return false;
        }

        if (::connect(_sockfd, (struct sockaddr*)&serverAddr,
                      sizeof(serverAddr)) < 0) {
            std::cerr << "[错误] 连接服务器失败: " << _ip << ":" << _port
                      << std::endl;
            close(_sockfd);
            _sockfd = -1;
            return false;
        }

        std::cout << "[信息] 已连接到服务器 " << _ip << ":" << _port
                  << std::endl;
        return true;
    }

    void disconnect() {
        if (_sockfd >= 0) {
            close(_sockfd);
            _sockfd = -1;
            std::cout << "[信息] 已断开连接" << std::endl;
        }
    }

    /**
     * @brief 发送 HTTP GET 请求到指定路径
     * @param path URL 路径，如 /suggest 或 /search
     * @param query 查询关键词
     * @return 是否发送成功
     */
    bool sendGetRequest(const std::string& path, const std::string& query) {
        if (_sockfd < 0) {
            std::cerr << "[错误] 未连接到服务器" << std::endl;
            return false;
        }

        // 构建 HTTP GET 请求
        std::string request = "GET " + path + "?q=" + urlEncode(query) + " HTTP/1.1\r\n"
                              "Host: " + _ip + "\r\n"
                              "Connection: keep-alive\r\n"
                              "Accept: application/json\r\n"
                              "\r\n";
        ssize_t sent = ::send(_sockfd, request.c_str(), request.size(), 0);
        if (sent < 0) {
            std::cerr << "[错误] 发送请求失败" << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief 发送词典搜索请求（关键词联想） GET /suggest?q=...
     */
    bool sendSuggestQuery(const std::string& query) {
        return sendGetRequest("/suggest", query);
    }

    /**
     * @brief 发送网页搜索请求 GET /search?q=...
     */
    bool sendSearchQuery(const std::string& query) {
        return sendGetRequest("/search", query);
    }

    /**
     * @brief 接收 HTTP 响应并提取 JSON 响应体
     * @param timeoutMs 超时毫秒数，0 表示不超时（阻塞）
     * @return 响应的 JSON 字符串，失败返回空字符串
     */
    std::string receiveResponse(int timeoutMs = 5000) {
        if (_sockfd < 0) {
            std::cerr << "[错误] 未连接到服务器" << std::endl;
            return "";
        }

        // 设置接收超时
        if (timeoutMs > 0) {
            struct timeval tv;
            tv.tv_sec = timeoutMs / 1000;
            tv.tv_usec = (timeoutMs % 1000) * 1000;
            setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        }

        char buffer[BUFFER_SIZE];
        std::string raw;
        ssize_t totalReceived = 0;

        // 第一阶段：读取直到找到 \r\n\r\n（HTTP 头结束标记）
        while (raw.find("\r\n\r\n") == std::string::npos) {
            ssize_t n = recv(_sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) {
                if (n == 0) {
                    std::cerr << "[错误] 服务器关闭了连接" << std::endl;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::cerr << "[错误] 接收响应超时" << std::endl;
                    } else {
                        std::cerr << "[错误] 接收响应失败" << std::endl;
                    }
                }
                close(_sockfd);
                _sockfd = -1;
                return "";
            }
            buffer[n] = '\0';
            raw.append(buffer, n);
            totalReceived += n;

            // 防止无限循环
            if (totalReceived > BUFFER_SIZE * 10) {
                std::cerr << "[错误] 响应数据过大" << std::endl;
                return "";
            }
        }

        // 解析 Content-Length
        size_t headerEnd = raw.find("\r\n\r\n");
        std::string headers = raw.substr(0, headerEnd);
        std::string body = raw.substr(headerEnd + 4);

        size_t contentLength = 0;
        size_t clPos = headers.find("Content-Length: ");
        if (clPos != std::string::npos) {
            contentLength = std::stoul(headers.substr(clPos + 16));
        }

        // 检查 HTTP 状态码
        size_t statusCodeStart = headers.find(' ') + 1;
        int statusCode = std::stoi(headers.substr(statusCodeStart, 3));

        // 第二阶段：如果 body 还不够，继续读取剩余内容
        while (body.size() < contentLength) {
            ssize_t n = recv(_sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (n <= 0) break;
            buffer[n] = '\0';
            body.append(buffer, n);
        }

        // 非 200 状态码打印警告
        if (statusCode != 200) {
            std::cerr << "[警告] 服务器返回状态码 " << statusCode << std::endl;
        }

        return body;
    }

    bool isConnected() const {
        return _sockfd >= 0;
    }

private:
    std::string _ip;
    unsigned short _port;
    int _sockfd;
};

// ============================================================
// 辅助函数：格式化打印 JSON（简易版）
// ============================================================
void printResponse(const std::string& jsonStr) {
    if (jsonStr.empty()) {
        std::cout << "[结果] 空响应" << std::endl;
        return;
    }

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "[响应]" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 简单的 JSON 格式化（缩进处理）
    int indent = 0;
    bool inString = false;
    for (size_t i = 0; i < jsonStr.size(); ++i) {
        char c = jsonStr[i];

        if (c == '"' && (i == 0 || jsonStr[i - 1] != '\\')) {
            inString = !inString;
            std::cout << c;
        } else if (inString) {
            std::cout << c;
        } else if (c == '{' || c == '[') {
            std::cout << c << '\n';
            ++indent;
            std::cout << std::string(indent * 2, ' ');
        } else if (c == '}' || c == ']') {
            std::cout << '\n';
            --indent;
            std::cout << std::string(indent * 2, ' ') << c;
        } else if (c == ',') {
            std::cout << c << '\n';
            std::cout << std::string(indent * 2, ' ');
        } else if (c == ':') {
            std::cout << c << ' ';
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            // 跳过空白
        } else {
            std::cout << c;
        }
    }

    std::cout << std::endl;
    std::cout << std::string(60, '=') << std::endl << std::endl;
}

// ============================================================
// 显示帮助
// ============================================================
void showHelp() {
    std::cout << R"(
══════════════════════════════════════════════════════════
             搜索引擎客户端 - 使用帮助
══════════════════════════════════════════════════════════

  查询方式:
    直接输入关键词       → 词典搜索（关键词联想 / 提示词）
    /search <关键词>     → 网页搜索（内容检索）

  HTTP API 端点:
    GET /suggest?q=关键词  → 词典搜索
    GET /search?q=关键词   → 网页搜索

  示例:
    > 搜索引擎            (词典搜索，返回前5条联想结果)
    > /search 搜索引擎     (网页搜索，返回网页内容)

  交互命令:
    /help 或 /h          显示此帮助
    /qps [关键词]        执行 20 秒 QPS 压测（默认关键词："搜索引擎"）
    /quit 或 /exit 或 /q 退出客户端
    
══════════════════════════════════════════════════════════
)" << std::endl;
}

// ============================================================
// QPS 压测函数：在指定时间内持续发送 HTTP GET 请求，统计 QPS
// ============================================================
void runBenchmark(const std::string& serverIp, unsigned short serverPort,
                  const std::string& query, int durationSec = 20) {
    SearchClient client(serverIp, serverPort);
    if (!client.connect()) {
        std::cerr << "[错误] QPS 测试：无法连接服务器" << std::endl;
        return;
    }

    std::cout << "\n══════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "  开始 QPS 压测 (HTTP)" << std::endl;
    std::cout << "  端点: /suggest?q=" << query << std::endl;
    std::cout << "  测试时长: " << durationSec << " 秒" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl
              << std::endl;

    auto startTime = std::chrono::steady_clock::now();
    long long totalRequests = 0;
    long long successRequests = 0;
    long long failedRequests = 0;
    auto endTime = startTime + std::chrono::seconds(durationSec);

    // 持续发送 HTTP 请求直到时间结束
    while (std::chrono::steady_clock::now() < endTime) {
        if (!client.isConnected()) {
            std::cerr << "[错误] QPS 测试：连接已断开，尝试重连..." << std::endl;
            if (!client.connect()) {
                std::cerr << "[错误] QPS 测试：重连失败，终止测试" << std::endl;
                break;
            }
        }

        totalRequests++;
        if (!client.sendSuggestQuery(query)) {
            failedRequests++;
            continue;
        }

        std::string response = client.receiveResponse(1000); // 1秒超时
        if (response.empty()) {
            failedRequests++;
        } else {
            successRequests++;
        }

        // 每 1000 次请求打印一次进度
        if (totalRequests % 1000 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            std::cout << "\r[进度] 已发送 " << totalRequests << " 次请求，"
                      << "用时 " << elapsed << " 秒" << std::flush;
        }
    }

    auto actualDuration = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - startTime).count();

    double qps = successRequests / actualDuration;

    std::cout << "\n\n══════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "  QPS 压测结果 (HTTP)" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "  查询词:           \"" << query << "\"" << std::endl;
    std::cout << "  实际测试时长:      " << std::fixed << std::setprecision(2)
              << actualDuration << " 秒" << std::endl;
    std::cout << "  总请求数:         " << totalRequests << std::endl;
    std::cout << "  成功请求数:       " << successRequests << std::endl;
    std::cout << "  失败请求数:       " << failedRequests << std::endl;
    std::cout << "  QPS:              " << std::fixed << std::setprecision(2)
              << qps << " 次/秒" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl
              << std::endl;

    client.disconnect();
}

// ============================================================
// 主函数
// ============================================================
int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string serverIp = "192.168.159.129";
    unsigned short serverPort = 8080;

    if (argc >= 2) {
        serverIp = argv[1];
    }
    if (argc >= 3) {
        serverPort = static_cast<unsigned short>(std::stoi(argv[2]));
    }

    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "            搜索引擎客户端 (HTTP) - 测试工具" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "  服务器: " << serverIp << ":" << serverPort << std::endl;
    std::cout << "  输入 /help 查看帮助，/quit 退出" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════"
              << std::endl
              << std::endl;

    // 连接服务器
    SearchClient client(serverIp, serverPort);
    if (!client.connect()) {
        return 1;
    }

    // 交互循环
    std::string input;
    while (true) {
        // 检查连接状态
        if (!client.isConnected()) {
            std::cerr << "[错误] 连接已断开" << std::endl;
            break;
        }

        std::cout << "> ";
        std::getline(std::cin, input);

        // 空输入跳过
        if (input.empty()) {
            continue;
        }

        // 检查特殊命令
        if (input == "/quit" || input == "/exit" || input == "/q") {
            std::cout << "[信息] 退出客户端" << std::endl;
            break;
        }

        if (input == "/help" || input == "/h") {
            showHelp();
            continue;
        }

        // QPS 压测命令
        if (input == "/qps" || input.substr(0, 5) == "/qps ") {
            std::string query;
            if (input == "/qps") {
                query = "搜索引擎";
            } else {
                query = input.substr(5);
            }
            runBenchmark(serverIp, serverPort, query, 20);
            continue;
        }

        // 网页搜索命令：/search <关键词>
        if (input.substr(0, 8) == "/search ") {
            std::string query = input.substr(8);
            if (query.empty()) {
                std::cout << "[信息] 用法: /search <关键词>" << std::endl;
                continue;
            }
            std::cout << "[信息] 正在网页搜索: \"" << query << "\" ..." << std::endl;
            if (!client.sendSearchQuery(query)) {
                std::cerr << "[错误] 发送搜索请求失败" << std::endl;
                continue;
            }

            std::string response = client.receiveResponse();
            if (response.empty()) {
                if (!client.isConnected()) {
                    std::cerr << "[错误] 与服务器的连接已断开，退出" << std::endl;
                    break;
                }
                continue;
            }
            printResponse(response);
            continue;
        }

        // 默认：词典搜索（关键词联想 / 提示词）
        std::cout << "[信息] 正在词典搜索: \"" << input << "\" ..." << std::endl;
        if (!client.sendSuggestQuery(input)) {
            std::cerr << "[错误] 发送查询失败" << std::endl;
            continue;
        }

        // 接收响应
        std::string response = client.receiveResponse();
        if (response.empty()) {
            // receiveResponse 内部已打印错误信息
            if (!client.isConnected()) {
                std::cerr << "[错误] 与服务器的连接已断开，退出" << std::endl;
                break;
            }
            continue;
        }

        // 打印响应
        printResponse(response);
    }

    client.disconnect();
    return 0;
}
