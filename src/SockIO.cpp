#include "SockIO.h"

SockIO::SockIO()
{
    fd=::socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1){
        perror("socket");
    }
}
SockIO::SockIO(int fd):
fd(fd)
{

}
SockIO::~SockIO()
{

}
int SockIO::readn(char *buf,int len){
    int cnt=0;
    while(cnt<len){
        int err=::recv(fd,buf,4096,0);
        if(err==-1){
            perror("recv");
        }
        buf+=err;
        cnt+=err;
    }
    return cnt;
}
int SockIO::writen(const char *buf, int len) {
    if (buf == nullptr || len <= 0) {
        return 0; // 无效输入，返回0表示写入0字节
    }
    int total_written = 0;
    int remaining = len;
    while (remaining > 0) {
        // 每次写入剩余数据（最多4096字节）
        int written = write(fd, buf + total_written, remaining > 4096 ? 4096 : remaining);
        if (written == -1) {
            // 处理中断错误（EINTR），可以重试
            if (errno == EINTR) {
                continue;
            }
            // 其他错误，返回已写入字节数（可能为部分写入）
            perror("write error");
            return total_written;
        } else if (written == 0) {
            // 写入0字节通常表示连接关闭
            break;
        }
        total_written += written;
        remaining -= written;
    }
    // 返回实际写入的总字节数（可能小于len，需调用者判断）
    return total_written;
}
int SockIO::readline(char* buf) {
    if (!buf) return -1; // 空指针检查
    
    int total = 0;
    char* p = buf;
    const int MAX_BUF_LEN = 4096; // 显式定义缓冲区大小，避免依赖类内定义
    int left = MAX_BUF_LEN - 1; // 预留终止符空间

    while (left > 0) {
        // 预览数据
        int peek = recv(fd, p, left, MSG_PEEK);
        if (peek < 0) {
            // 处理非阻塞模式下的暂时无数据情况
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // 重试
            }
            return -1; // 其他错误
        } else if (peek == 0) {
            // 对方关闭连接
            break;
        }
        
        // 查找换行符或终止符
        int i;
        for (i = 0; i < peek; ++i) {
            if (p[i] == '\n' || p[i] == '\0') {
                // 找到分隔符，读取到分隔符（包含分隔符）
                int n = recv(fd, p, i + 1, 0);
                if (n <= 0) {
                    // 读取实际数据时出错
                    if (n == 0) break; // 连接关闭
                    return -1;
                }
                p[i + 1] = '\0'; // 添加终止符
                return total + n;
            }
        }
        
        // 未找到分隔符，读取全部预览数据
        int n = recv(fd, p, peek, 0);
        if (n <= 0) {
            if (n == 0) break; // 连接关闭
            return -1;
        }
        
        total += n;
        p += n;
        left -= n;
    }

    // 处理缓冲区已满或连接关闭的情况
    *p = '\0'; // 确保字符串终止
    return total > 0 ? total : (total == 0 ? 0 : -1);
}
