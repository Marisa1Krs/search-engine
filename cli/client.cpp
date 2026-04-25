/**
 * @file client.cpp
 * @brief 搜索引擎客户端 - 用于连接搜索服务端进行用例测试
 *
 * 用法: ./client <ip> <port>
 * 默认连接: 192.168.159.129:8080
 *
 * 协议:
 *   - 发送: 查询字符串（以 '/' 开头触发网页搜索，否则为词典搜索）
 *   - 接收: JSON 字符串 + '\n' 结尾
 *
 * 交互命令:
 *   - 输入查询字符串直接搜索
 *   - /quit 或 /exit 退出
 *   - /help 显示帮助
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

constexpr int BUFFER_SIZE = 65536;

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

    bool sendQuery(const std::string& query) {
        if (_sockfd < 0) {
            std::cerr << "[错误] 未连接到服务器" << std::endl;
            return false;
        }

        std::string msg = query + "\n";
        ssize_t sent = ::send(_sockfd, msg.c_str(), msg.size(), 0);
        if (sent < 0) {
            std::cerr << "[错误] 发送请求失败" << std::endl;
            return false;
        }
        return true;
    }

    /**
     * @brief 接收一行以 '\n' 结尾的响应
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
        std::string result;
        ssize_t totalReceived = 0;

        while (true) {
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
            result.append(buffer, n);
            totalReceived += n;

            // 检查是否接收到了完整的行
            if (result.back() == '\n') {
                // 去掉末尾的 '\n'
                result.pop_back();
                break;
            }

            // 防止无限循环
            if (totalReceived > BUFFER_SIZE * 10) {
                std::cerr << "[错误] 响应数据过大" << std::endl;
                return "";
            }
        }

        return result;
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
    直接输入关键词       → 词典搜索（DictProducer）
    以 / 开头            → 网页搜索（PageLibPreprocessor）

  示例:
    > 搜索引擎            (词典搜索，返回前5条)
    > /C3-Art0002         (网页搜索，按文档ID查找)

  交互命令:
    /help 或 /h          显示此帮助
    /quit 或 /exit 或 /q 退出客户端
    
══════════════════════════════════════════════════════════
)" << std::endl;
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
    std::cout << "            搜索引擎客户端 - 测试工具" << std::endl;
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

        // 发送查询
        std::cout << "[信息] 正在查询: \"" << input << "\" ..." << std::endl;
        if (!client.sendQuery(input)) {
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