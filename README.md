# 中文搜索引擎

基于 C++11 实现的轻量级中文搜索引擎，提供**词典搜索**（关键词联想）和**网页搜索**（内容检索）两种功能。使用 Reactor 网络模型 + 线程池处理并发请求，集成 cppjieba 分词和 simhash 去重。

---

## 目录

- [系统架构](#系统架构)
- [项目目录结构](#项目目录结构)
- [环境要求](#环境要求)
- [构建与运行](#构建与运行)
- [配置文件](#配置文件)
- [协议与 API](#协议与-api)
- [客户端使用](#客户端使用)
- [核心模块说明](#核心模块说明)
- [数据流](#数据流)

---

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    客户端 (cli/client)                    │
│        HTTP GET 请求 → JSON 响应（HTTP/1.1）              │
└──────────────────────┬──────────────────────────────────┘
                        │ TCP (IPv4)
┌──────────────────────▼──────────────────────────────────┐
│                    服务端 (src/)                          │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐  │
│  │  TcpServer   │  │  threadpool  │  │   EventLoop     │  │
│  │ (多 Reactor) │─▶│  (线程池)    │  │ (主+子事件循环)  │  │
│  └──────┬───────┘  └──────┬───────┘  └────────────────┘  │
│         │                 │                               │
│  ┌──────▼─────────────────▼───────┐                      │
│  │    onMessage() HTTP 路由       │                      │
│  │  recvHttp() → parseHttpGet()   │                      │
│  │  /suggest → DictProducer       │                      │
│  │  /search  → PageLibPreprocessor │                      │
│  └─────────────────────────────────┘                      │
│                                                           │
│  ┌──────────────────┐  ┌──────────────────────┐          │
│  │   DictProducer    │  │ PageLibPreprocessor  │          │
│  │   (词典构建/搜索)  │  │ (网页库/倒排索引)     │          │
│  └──────────────────┘  └──────────────────────┘          │
│  ┌──────────────────┐  ┌──────────────────────┐          │
│  │   SplitTool       │  │       Configer       │          │
│  │ (jieba 分词封装)   │  │   (配置解析器)       │          │
│  └──────────────────┘  └──────────────────────┘          │
└───────────────────────────────────────────────────────────┘
```

---

## 项目目录结构

```
search-engine/
├── cli/                          # 客户端
│   ├── client.cpp                # TCP 客户端实现
│   └── CMakeLists.txt
├── config/
│   └── serch.conf                # 主配置文件（所有可调参数）
├── data/
│   ├── newripepage.dat           # 处理后网页库（XML 格式）
│   ├── yuliao/
│   │   ├── xml/                  # RSS/XML 原始语料（分频道）
│   │   ├── chinese/              # 中文语料文本
│   │   ├── english/              # 英文语料文本
│   │   ├── stop_words_zh.txt     # 中文停用词表
│   │   ├── stop_words_eng.txt    # 英文停用词表
│   │   └── ...
│   └── doc/
├── dict/                         # jieba 分词词典
│   ├── jieba.dict.utf8           # 核心词典
│   ├── hmm_model.utf8            # HMM 模型
│   ├── user.dict.utf8            # 用户自定义词典
│   ├── idf.utf8                  # IDF 权重
│   └── stop_words.utf8           # 停用词
├── include/                      # 头文件
│   ├── Acceptor.h                # 连接接收器
│   ├── Configer.h                # 配置解析器
│   ├── DictProducer.h            # 词典生产者
│   ├── EventLoop.h               # 事件循环
│   ├── InetAddress.h             # IP 地址封装
│   ├── mylog.h                   # 日志系统
│   ├── PageLib.h                 # 网页库
│   ├── PageLibPreprocessor.h     # 网页库预处理（倒排索引）
│   ├── Socket.h / SockIO.h       # Socket 封装
│   ├── SplitTool.h               # 分词工具封装
│   ├── Task.h                    # 任务封装
│   ├── taskQueue.h               # 线程安全任务队列
│   ├── TcpConnetion.h            # TCP 连接封装
│   ├── TcpServer.h               # TCP 服务器（Reactor）
│   ├── threadpool.h              # 线程池
│   ├── WebPage.h                 # 网页文档模型
│   ├── cppjieba/                 # jieba 分词库（头文件）
│   ├── json/                     # JSON 库（nlohmann）
│   └── simhash/                  # simhash 去重库
├── src/                          # 源文件
│   ├── main.cpp                  # 入口，初始化+启动
│   ├── Configer.cpp              # 配置解析实现
│   ├── DictProducer.cpp          # 词典构建
│   ├── PageLibPreprocessor.cpp   # 倒排索引构建+搜索
│   ├── SplitTool.cpp             # 分词封装
│   ├── WebPage.cpp               # 网页处理
│   ├── TcpServer.cpp             # 服务器网络层
│   ├── TcpConnetion.cpp          # 连接管理
│   ├── threadpool.cpp            # 线程池实现
│   ├── ...                       # 其他网络组件
├── log/                          # 运行时日志输出
├── build/                        # 编译输出目录
├── CMakeLists.txt                # 顶级 CMake 配置
└── README.md                     # 本文件
```

---

## 环境要求

| 组件 | 版本 |
|------|------|
| 操作系统 | Linux (Ubuntu 18.04+) |
| 编译器 | GCC 4.8.4+ (需支持 C++11) |
| CMake | 3.10+ |
| tinyxml2 | 用于 XML 解析（编译时链接） |

### 安装依赖

```bash
# Ubuntu / Debian
sudo apt-get install build-essential cmake libtinyxml2-dev
```

---

## 构建与运行

### 1. 编译

```bash
cd search-engine
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

编译产物：
- `build/server` — 服务端可执行程序
- `build/cli/client` — 客户端可执行程序

### 2. 配置

编辑 [`config/serch.conf`](config/serch.conf) 确保所有路径指向正确的目录。

**首次运行前需要确保：**
- XML 语料库在 [`data/yuliao/xml/`](data/yuliao/xml/) 目录下
- jieba 词典文件在 [`dict/`](dict/) 目录下
- 停用词文件在 [`data/yuliao/`](data/yuliao/) 目录下

### 3. 运行服务端

```bash
cd build
./server
```

启动过程：
1. 加载配置 & 初始化日志
2. **启动 TCP 服务器 & 线程池**（此时 `g_serverReady = false`，客户端连接会被拒绝并提示"正在初始化"）
3. 构建英文/中文词典
4. 解析 XML 语料，生成网页库和倒排索引
5. `g_serverReady = true`，开始正常处理客户端请求

### 4. 运行客户端

```bash
cd build
./cli/client <服务器IP> <端口>
# 默认连接 192.168.159.129:8080
# 示例：./cli/client 127.0.0.1 8080
```

---

## 配置文件

配置文件 [`config/serch.conf`](config/serch.conf) 采用 `key:value` 格式，支持 `#` 注释（行首注释和内联注释）。

### 完整配置项

```ini
# ==================== 路径配置 ====================
xmlPath:           # XML 语料库目录
webPagePath:       # 处理后网页库文件路径
chineseDir:        # 中文语料目录（词典构建）
englishDir:        # 英文语料目录
stopWordsCnPath:   # 中文停用词路径
stopWordsEnPath:   # 英文停用词路径
logPath:           # 日志文件路径
jiebaDictPath:     # jieba 核心词典
hmmPath:           # jieba HMM 模型
userDictPath:      # jieba 用户词典
idfPath:           # jieba IDF 路径
jiebaStopWordPath: # jieba 停用词路径

# ==================== 网络配置 ====================
serverIp:          # 监听 IP 地址
serverPort:        # 监听端口号
subEventLoopCount: # 子 EventLoop 数量（多 Reactor 模型）

# ==================== 线程池配置 ====================
threadPoolThreadCount: # 工作线程数
threadPoolQueueSize:   # 任务队列最大长度

# ==================== 算法参数 ====================
topNVal:           # 关键词提取 / simhash 的 TopN 值
simhashDistance:   # 海明距离阈值（去重相似度）
englishBufSize:    # 英文缓冲区大小
chineseBufSize:    # 中文缓冲区大小
stopFileBufSize:   # 停用词文件缓冲区大小

# ==================== 日志配置 ====================
logBufferSize:     # 日志缓冲区大小（条目数）
logLevel:          # 日志级别：0=DEBUG 1=INFO 2=WARN 3=ERROR
```

---

## 协议与 API

### 通信协议

- **传输层**: TCP
- **应用层**: HTTP/1.1
- **编码**: UTF-8（URL 中的非 ASCII 字符需 Percent-Encoding）
- **请求格式**: HTTP GET 请求
- **响应格式**: HTTP 响应，`Content-Type: application/json; charset=utf-8`

### API 端点

#### 1. 词典搜索（关键词联想 / 提示词）

**端点**: `GET /suggest?q=<关键词>`

**请求示例**:

```
GET /suggest?q=%E6%90%9C%E7%B4%A2 HTTP/1.1
Host: 127.0.0.1:8080
Connection: keep-alive
Accept: application/json
```

**响应示例**:

```json
["搜索引擎", "搜索", "引擎", "检索", "查询"]
```

**实现**: [`DictProducer::find()`](include/DictProducer.h:27) — 基于索引找出候选词，计算编辑距离后返回 Top-K（默认 5 个）。

---

#### 2. 网页搜索（内容检索）

**端点**: `GET /search?q=<关键词>`

**请求示例**:

```
GET /search?q=%E7%99%8C%E7%97%87 HTTP/1.1
Host: 127.0.0.1:8080
Connection: keep-alive
Accept: application/json
```

**响应示例**:

```json
{
    "title": "文章标题",
    "url": "原文链接",
    "content": "摘要文本..."
}
```

**实现**: [`PageLibPreprocessor::find()`](include/PageLibPreprocessor.h:25)

搜索流程：
1. 用 jieba 的 `KeywordExtractor` 提取关键词（Top-5）
2. 在倒排索引中查找每个关键词对应的文档集合
3. 取多个关键词文档集合的交集
4. 计算余弦相似度，返回最相关的一篇文档

---

#### 3. 根路径（帮助信息）

**端点**: `GET /`

返回可用 API 端点的 JSON 描述。

---

### 状态码

| 状态码 | 含义 |
|--------|------|
| 200 OK | 请求成功 |
| 503 Service Unavailable | 服务器正在初始化倒排索引，请稍后再试 |

### 查询示例

| `curl` 命令 | 描述 |
|-------------|------|
| `curl "http://127.0.0.1:8080/suggest?q=搜索"` | 词典搜索 |
| `curl "http://127.0.0.1:8080/search?q=慢性病"` | 网页搜索 |
| `curl "http://127.0.0.1:8080/"` | 查看 API 帮助 |

---

## 客户端使用

客户端提供交互式命令行界面，基于 HTTP 协议与服务端通信。

```bash
./cli/client 192.168.159.129 8080
```

### 交互命令

| 命令 | 功能 |
|------|------|
| `<关键词>` | 词典搜索（关键词联想），发送 `GET /suggest?q=<关键词>` |
| `/search <关键词>` | 网页搜索（内容检索），发送 `GET /search?q=<关键词>` |
| `/qps [关键词]` | 执行 20 秒 QPS 压测（HTTP，默认关键词"搜索引擎"） |
| `/help` 或 `/h` | 显示帮助 |
| `/quit` 或 `/exit` 或 `/q` | 退出客户端 |

### 输出格式

响应以格式化 JSON 输出，外壳为 `===` 分隔线。

---

## 核心模块说明

### [`Configer`](include/Configer.h) — 配置解析器

- 读取 `serch.conf`，每行 `key:value` 格式
- 跳过 `#` 开头的注释行，支持内联注释
- 提供 `getConfigMap()` 获取键值对映射

### [`SplitTool`](include/SplitTool.h) — 分词工具封装

- 封装 cppjieba 的 `Jieba` 和 simhash 的 `Simhasher`
- `cut()` — 调用 jieba `MixSegment` 算法分词
- `extract()` — 调用 `KeywordExtractor` 提取关键词及权重
- `make()` — 计算文本的 simhash 指纹（用于去重）
- 单例模式，通过 `init(Configer&)` 从配置文件读取词典路径

### [`DictProducer`](include/DictProducer.h) — 词典生产者

- **英文词典**: 读取英文语料 → `washWordsEn()`（大写转小写，非字母变空格）→ `loadDict()` 统计词频
- **中文词典**: 读取中文语料 → `washWordsCn()`（jieba 分词）→ `loadDict()`
- **搜索**: `find()` — 基于字符索引检索候选词，编辑距离排序
- 单例模式，配置值在 `init()` 时从 Configer 读取

### [`PageLibPreprocessor`](include/PageLibPreprocessor.h) — 网页库预处理

- **`readInfoFromFile()`**: 解析 XML 语料（RSS 格式）→ 提取 title/content/url → 写入 `newripepage.dat`（网页库）→ 构建 `_offsetLib`（偏移库）
- **`buildInvertIndex()`**: 两遍扫描构建倒排索引
  - 第一遍：分词 → 统计每篇文档的词频（TF）
  - 第二遍：计算 IDF → 计算 TF-IDF 权重 → 存入 `_invertIndex`
- **`find()`**: 关键词提取 → 倒排索引查找 → 文档交集 → 余弦相似度排序 → 返回最相关文档

### 网络层

| 组件 | 功能 |
|------|------|
| [`TcpServer`](include/TcpServer.h) | 多 Reactor 模型服务器，组合 Acceptor + 主 EventLoop + EventLoopGroup |
| [`Acceptor`](include/Acceptor.h) | 监听 socket，接受新连接 |
| [`EventLoop`](include/EventLoop.h) | epoll 事件循环，管理 IO 事件；支持主/子两种模式 |
| [`EventLoopGroup`](include/EventLoopGroup.h) | 子 EventLoop 管理器，通过轮询（Round-Robin）分配新连接到子 Reactor |
| [`TcpConnetion`](include/TcpConnetion.h) | 封装 TCP 连接，处理读写；支持 HTTP 请求读取（`recvHttp()`） |
| [`threadpool`](include/threadpool.h) | 线程池，异步处理业务逻辑 |

### [`mylog`](include/mylog.h) — 日志系统

- 双缓冲异步日志，生产者-消费者模型
- 支持 5 个级别：DEBUG / INFO / WARN / ERROR / FATAL
- 通过宏 `LOG_DEBUG` / `LOG_INFO` / `LOG_WARN` / `LOG_ERROR` 调用

---

## 数据流

### 启动流程

```
main()
  ├─ Configer(confPath)            ← 读取配置文件
  ├─ mylog::init()                 ← 初始化日志
  │
  ├─ TcpServer()                   ← 创建服务器（主 EventLoop + 子 EventLoopGroup）
  ├─ threadpool::start()           ← 启动线程池
  ├─ server->start() (后台线程)     ← 启动 TCP 服务器（主 EventLoop 开始 accept）
  │   ├─ 主 EventLoop accept 新连接
  │   ├─ 轮询（Round-Robin）分配给子 EventLoop
  │   └─ 子 EventLoop 处理 IO 读写
  │
  ├─ [此时 g_serverReady=false]     ← 拒绝连接，提示"正在初始化"
  │
  ├─ DictProducer::init(con)       ← 初始化分词器 + 词典
  │   └─ SplitTool::init(con)      ← 从配置读取 jieba 路径
  ├─ buildEnDict()                 ← 构建英文词典
  ├─ buildCnDict()                 ← 构建中文词典
  ├─ PageLibPreprocessor::init(con)
  ├─ doProcess()
  │   ├─ readInfoFromFile()        ← 解析 XML → 网页库
  │   └─ buildInvertIndex()        ← 构建倒排索引
  │
  ├─ g_serverReady = true          ← 服务器就绪，开始处理请求
  └─ serverThread.join()
```

### 搜索流程

```
客户端输入查询
       │
       ▼
TcpServer::onMessage()
       │
       ├─ con->recvHttp()           ← 读取完整 HTTP 请求头（直到 \r\n\r\n）
       ├─ parseHttpGet()            ← 解析路径和 q 参数
       │
       ├─ g_serverReady == false?
       │      └── 是 → 返回 503 Service Unavailable
       │
       ├─ query 为空?
       │      └── 是 → 返回 API 端点帮助信息
       │
       ▼
handleHttpRequest(path, query, con)  ← 线程池异步处理
       │
       ├── path == "/suggest"?
       │      └── 是 → DictProducer::find()
       │                 ├── 字符索引检索候选词
       │                 ├── 编辑距离排序
       │                 └── 返回 JSON 数组
       │
       ├── path == "/search"?
       │      └── 是 → PageLibPreprocessor::find()
       │                 ├── KeywordExtractor 提取关键词
       │                 ├── 倒排索引查找文档
       │                 ├── 取文档交集
       │                 └── 余弦相似度排序 → 返回 JSON
       │
       └── 其他路径 → 返回 JSON 帮助信息
       │
       ▼
  buildHttpResponse(json)          ← 包装 HTTP 响应（含 Content-Length 等头）
       │
       ▼
  con->sendInLoop(httpResponse)    ← 发送 HTTP 响应
```

---

## 许可证

本项目基于 MIT 许可证开源。
