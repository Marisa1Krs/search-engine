#ifndef MYLOG_H
#define MYLOG_H

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <cstring>
#include <ctime>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>

// 日志级别枚举
enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_LEVEL_MAX
};

// 日志级别名称（与枚举顺序一致）
static const char* g_log_level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

class mylog {
private:
    mylog() = default;
    mylog(const std::string& filePath, size_t bufSize, LogLevel logLevel);
    ~mylog();

public:
    // 禁止拷贝构造和赋值
    mylog(const mylog&) = delete;
    mylog& operator=(const mylog&) = delete;

    // 初始化日志（线程安全）
    static void init(const std::string& filePath = "", size_t bufSize = 1024, LogLevel logLevel = LOG_INFO);
    // 获取单例指针
    static mylog* getPtr();
    // 设置日志级别
    void setLogLevel(LogLevel logLevel) { this->logLevel = logLevel; }
    // 设置缓冲区大小
    void setBufferSize(size_t size) { this->bufSize = size; }
    // 写入日志（可变参数）
    void writeLog(LogLevel level, const char* file, int line, const char* fmt, ...);

private:
    // 写入缓冲区A
    void writeBufferA(const std::string& logtext);
    // 后台工作线程（消费日志）
    void worker();
    // 创建目录（辅助函数）
    bool createDirectory(const std::string& path);

private:
    static mylog* _ptr;          // 单例指针
    static std::mutex _initMutex;// 初始化锁（确保单例线程安全）
    std::string logDir;          // 日志文件路径
    size_t bufSize;              // 缓冲区最大条目数
    LogLevel logLevel;           // 日志级别阈值
    std::thread helper;          // 后台消费线程
    bool running;                // 运行标志
    std::vector<std::string> bufferA;  // 生产者缓冲区
    std::vector<std::string> bufferB;  // 消费者缓冲区
    std::mutex logMutex;         // 缓冲区锁
    std::condition_variable isEmpty;   // 缓冲区非空条件
    std::condition_variable isFull;    // 缓冲区未满条件
    int fd;                      // 日志文件描述符
};

// 日志宏定义（简化调用，自动传入 __FILE__ 和 __LINE__）
#define LOG_DEBUG(fmt, ...) mylog::getPtr()->writeLog(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  mylog::getPtr()->writeLog(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  mylog::getPtr()->writeLog(LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) mylog::getPtr()->writeLog(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) mylog::getPtr()->writeLog(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif // MYLOG_H