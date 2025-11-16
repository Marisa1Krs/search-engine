#include "mylog.h"
#include <stdarg.h>

// 初始化静态成员
mylog* mylog::_ptr = nullptr;
std::mutex mylog::_initMutex;

// 构造函数（创建目录+打开文件）
mylog::mylog(const std::string& filePath, size_t bufSize, LogLevel logLevel)
    : logDir(filePath), bufSize(bufSize), logLevel(logLevel),
      running(true), fd(-1) {
    // 若指定文件路径，创建目录并打开文件
    if (!filePath.empty()) {
                // 提取目录部分（如 "/a/b/log.txt" → "/a/b"）
        size_t pos = filePath.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dir = filePath.substr(0, pos);
            if (!createDirectory(dir)) {
                std::cerr << "[mylog] 目录创建失败：" << dir << std::endl;
            }
        }

        // 打开日志文件（O_CREAT|O_RDWR|O_APPEND，权限 0666）
        fd = open(filePath.c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);
        if (fd == -1) {
            std::cerr << "[mylog] 日志文件打开失败！path=" << filePath
                      << ", errno=" << errno << ", msg=" << strerror(errno) << std::endl;
        } else {
            std::cout << "[mylog] 日志文件打开成功：" << filePath << ", fd=" << fd << std::endl;
        }
    } else {
        std::cout << "[mylog] 未指定日志文件，仅输出到终端" << std::endl;
    }

    // 启动后台消费线程（必须在所有初始化完成后启动）
    helper = std::thread(std::bind(&mylog::worker, this));
}

// 析构函数（安全停止线程+释放资源）
mylog::~mylog() {
    // 1. 标记停止，唤醒消费线程
    running = false;
    isEmpty.notify_one();  // 唤醒可能阻塞在 "isEmpty" 的线程
    isFull.notify_one();   // 唤醒可能阻塞在 "isFull" 的线程

    // 2. 等待后台线程结束（确保所有日志消费完成）
    if (helper.joinable()) {
        helper.join();
    }

    // 3. 关闭文件描述符
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    std::cout << "[mylog] 日志模块已关闭" << std::endl;
}

// 初始化单例（线程安全）
void mylog::init(const std::string& filePath, size_t bufSize, LogLevel logLevel) {
    if (_ptr == nullptr) {
        std::lock_guard<std::mutex> lock(_initMutex);  // 双重检查锁定，避免并发初始化
        if (_ptr == nullptr) {
            _ptr = new mylog(filePath, bufSize, logLevel);
        }
    }
}

// 获取单例指针
mylog* mylog::getPtr() {
    if (_ptr == nullptr) {
        std::cerr << "[mylog] 错误：未调用 init() 初始化日志模块！" << std::endl;
        abort();  // 未初始化直接崩溃，避免后续空指针访问
    }
    return _ptr;
}

// 写入生产者缓冲区 A（处理满缓冲阻塞）
void mylog::writeBufferA(const std::string& logtext) {
    std::unique_lock<std::mutex> lock(logMutex);
    // 缓冲区满时阻塞，直到消费者通知有空闲
    while (bufferA.size() >= bufSize && running) {
        isFull.wait(lock);
    }
    // 若已停止，直接丢弃日志
    if (!running) {
        return;
    }
    bufferA.push_back(logtext);
    lock.unlock();  // 提前解锁，减少锁持有时间

    // 通知消费者：缓冲区非空（必须在解锁后调用，避免唤醒丢失）
    isEmpty.notify_one();
}

// 后台消费线程（核心修复：规范条件变量使用）
void mylog::worker() {
    while (running) {
        std::unique_lock<std::mutex> lock(logMutex);

        // 修复1：缓冲区为空时阻塞，直到被唤醒（同时检查 running）
        while (bufferA.empty() && running) {
            isEmpty.wait(lock);  // 等待 "非空" 或 "停止" 信号
        }

        // 修复2：处理残留日志（即使 running 为 false，也要消费完 bufferA）
        if (!bufferA.empty()) {
            bufferA.swap(bufferB);  // 交换缓冲区，批量消费（解锁后处理）
        }

        // 解锁：让生产者可以继续写入 bufferA
        lock.unlock();

        // 消费 bufferB 中的日志（无锁操作，提高效率）
        if (!bufferB.empty()) {
            for (const auto& log : bufferB) {
                // 控制台输出：强制刷新，避免 VS Code 缓存
               printf("%s\n",log.c_str());
                
                // 文件写入：fd 有效时才写，避免无效调用
                if (fd != -1) {
                    ssize_t write_len = ::write(fd, log.c_str(), log.size());
                    if (write_len == -1) {
                        std::cerr << "[mylog] 文件写入失败！errno=" << errno 
                                  << ", msg=" << strerror(errno) << std::endl;
                    }
                }
            }
            bufferB.clear();  // 清空缓冲区，准备下次交换

            // 通知生产者：缓冲区有空位，可继续写入
            isFull.notify_one();
        }
    }

    // 最终检查：退出前消费完 bufferA 中剩余的日志（避免丢失）
    std::unique_lock<std::mutex> lock(logMutex);
    if (!bufferA.empty()) {
        for (const auto& log : bufferA) {
            std::cout << log;
            std::cout.flush();
            if (fd != -1) {
                ::write(fd, log.c_str(), log.size());
            }
        }
        bufferA.clear();
    }
    std::cout << "[mylog] 消费线程已退出" << std::endl;
}

// 写入日志（处理可变参数+日志格式拼接）
void mylog::writeLog(LogLevel level, const char* file, int line, const char* fmt, ...) {
    // 1. 校验参数：日志级别低于阈值/参数无效，直接返回
    if (level < logLevel || level >= LOG_LEVEL_MAX || fmt == nullptr) {
        return;
    }

    // 2. 拼接日志头部（时间+级别+文件行号）
    char log_buf[4096] = {0};
    size_t buf_pos = 0;

    // 2.1 获取当前时间（精确到毫秒）
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm;
    localtime_r(&ts.tv_sec, &tm);  // 线程安全的时间转换

    char time_buf[32] = {0};
    snprintf(time_buf, sizeof(time_buf),
             "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000);

    // 2.2 拼接时间+级别+文件行号（提取文件名，去掉路径）
    const char* filename = strrchr(file, '/') ? strrchr(file, '/') + 1 : file;
    buf_pos += snprintf(log_buf + buf_pos, sizeof(log_buf) - buf_pos,
                       "Marisa⭐Daze[%s] [%s] [%s:%d] ",
                       time_buf, g_log_level_names[level], filename, line);

    // 3. 拼接日志内容（处理可变参数）
    va_list args;
    va_start(args, fmt);
    buf_pos += vsnprintf(log_buf + buf_pos, sizeof(log_buf) - buf_pos, fmt, args);
    va_end(args);

    // 4. 补充换行符（确保每条日志独立一行）
    if (buf_pos < sizeof(log_buf) - 1) {
        log_buf[buf_pos++] = '\n';
        log_buf[buf_pos] = '\0';
    }

    // 5. 写入生产者缓冲区
    writeBufferA(std::string(log_buf));
}

// 辅助函数：递归创建目录（支持多级目录）
bool mylog::createDirectory(const std::string& path) {
    // 目录已存在，直接返回成功
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }

    // 递归创建父目录（如 "a/b/c" → 先创建 "a"，再 "a/b"，最后 "a/b/c"）
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string parent_dir = path.substr(0, pos);
        if (!createDirectory(parent_dir)) {
            return false;
        }
    }

    // 创建当前目录（权限 0755：所有者读写执行，其他读执行）
    if (mkdir(path.c_str(), 0755) == -1) {
        std::cerr << "[mylog] 创建目录失败！path=" << path
                  << ", errno=" << errno << ", msg=" << strerror(errno) << std::endl;
        return false;
    }
    return true;
}