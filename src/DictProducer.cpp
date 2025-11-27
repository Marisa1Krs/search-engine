#include "DictProducer.h"

using namespace std;
const char *banPathEn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_eng.txt";
const char *banPathCn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_zh.txt";

// 后续改成配置文件
DictProducer* DictProducer::_ptr=nullptr;
DictProducer::DictProducer(const string &dir)
    : _files(), _dict()
{
    LOG_INFO("Create DictProducer ,dir is %s", dir.c_str());
    DIR *fileDir = opendir(dir.c_str());
    if (fileDir == nullptr)
    {
        LOG_ERROR("open filedir fall,path is %s", dir.c_str());
        perror("opendir");
    }
    struct dirent *enterDir;
    while ((enterDir = readdir(fileDir)) != NULL)
    {

        if (strcmp(enterDir->d_name, ".") == 0 || strcmp(enterDir->d_name, "..") == 0)
        {
            continue;
        }
        if (enterDir->d_type == DT_REG)
        {
            string temp = dir + "/" + string(enterDir->d_name);
            _files.push_back(temp);
            LOG_DEBUG("file is pushed back %s", temp.c_str());
        }
    }
} // 英文
DictProducer::DictProducer(const string &dir, SplitTool *tool)
    : _files(), _splitTool(tool), _dict()
{
    LOG_DEBUG("Create DictProducer ,dir is %s", dir.c_str());
    DIR *fileDir = opendir(dir.c_str());
    if (fileDir == nullptr)
    {
        LOG_ERROR("open filedir fall,path is %s", dir.c_str());
        perror("opendir");
    }
    struct dirent *enterDir;
    while ((enterDir = readdir(fileDir)) != NULL)
    {

        if (strcmp(enterDir->d_name, ".") == 0 || strcmp(enterDir->d_name, "..") == 0)
        {
            continue;
        }
        if (enterDir->d_type == DT_REG)
        {
            string temp = dir + "/" + string(enterDir->d_name);
            _files.push_back(temp);
            LOG_DEBUG("file is pushed back %s", temp.c_str());
        }
    }
} // 中文

DictProducer::~DictProducer()
{
}
void DictProducer::loadDict(char *words, unordered_map<string, int> &mp,size_t size)
{
    string tempWord;
    for (int i = 0; i < size; i++)
    {
        if (words[i] == ' ' && tempWord.size() && tempWord != " ")
        {
            if (mp.find(tempWord) == mp.end())
            {
                int flag = _dict.size();
                for (int i = 0; i < _dict.size(); i++)
                {
                    if (_dict[i].first == tempWord)
                    {
                        _dict[i].second++;
                        flag = i;
                        break;
                    }
                }
                if (flag == _dict.size())
                {
                    _dict.push_back({tempWord, 1});
                }
                for (int i = 0; i < tempWord.size();)
                {
                    if ((tempWord[i] & 0x80) == 0)
                    {
                        _index[tempWord.substr(i, 1)].insert(flag);
                        i++;
                    }
                    else{
                        _index[tempWord.substr(i, 3)].insert(flag);
                        i+=3;
                    }
                }
            }
            tempWord.clear();
        }
        else if (words[i] == ' ')
        {
        }
        else
        {
            tempWord.push_back(words[i]);
        }
    }
}
void DictProducer::washWordsEn(char *words)
{
    for (int i = 0; i < 6488671; i++)
    {
        if (words[i] >= 'a' && words[i] <= 'z')
        {
        }
        else if (words[i] >= 'A' && words[i] <= 'Z')
        {
            words[i] = words[i] + 32;
        }
        else
        {
            words[i] = ' ';
        }
    }
}
void DictProducer::washWordsCn(char* words){
    string temp(words);
    vector<string> cutResult=SplitTool::getPtr()->cut(temp);
    temp.clear();
    for(auto &t:cutResult){
        temp+=t;
        temp.push_back(' ');
    }
    strcpy(words,temp.c_str());
}
void DictProducer::buildEnDict()
{
    int banFd = open(banPathEn, O_RDONLY); // 加载停用词
    char buf[8192];
    memset(buf, 0, 8192);
    ::read(banFd, buf, 8192);
    unordered_map<string, int> mp;
    string tempWord;
    for (int i = 0; i < 8192; i++)
    {
        if (buf[i] == '\r')
        {
            i++;
            if (tempWord.size())
            {
                mp[tempWord] = 1;
            }
            LOG_DEBUG("word %s is banned", tempWord.c_str());
            tempWord.clear();
        }
        else
        {
            tempWord.push_back(buf[i]);
        }
    }
    char englishBuf[6488666 + 5];
    memset(englishBuf, 0, 6488666 + 5);
    for (auto &t : _files)
    {
        int readFd = open(t.c_str(), O_RDONLY);
        if (readFd == -1)
        {
            LOG_ERROR("read fall %s", t.c_str());
            perror("read");
        }
        ::read(readFd, englishBuf, 6488666 + 5);
        washWordsEn(englishBuf);
        LOG_DEBUG("wash success path:%s",t.c_str());
        loadDict(englishBuf, mp ,6488666 + 5);
        LOG_DEBUG("load success path:%s",t.c_str());
        memset(englishBuf, 0, 6488666 + 5);
        close(readFd);
    }
}
void DictProducer::buildCnDict() {
    int banFd = open(banPathCn, O_RDONLY); // 加载停用词
    char buf[8192];
    memset(buf, 0, 8192);
    ::read(banFd, buf, 8192);
    unordered_map<string, int> mp;
    string tempWord;
    for (int i = 0; i < 8192; i++)
    {
        if (buf[i] == '\r')
        {
            i++;
            if (tempWord.size())
            {
                mp[tempWord] = 1;
            }
            LOG_DEBUG("word %s is banned", tempWord.c_str());
            tempWord.clear();
        }
        else
        {
            tempWord.push_back(buf[i]);
        }
    }
    char chineseBuf[100000];
    memset(chineseBuf, 0, 100000);
    for (auto &t : _files)
    {
        int readFd = open(t.c_str(), O_RDONLY);
        if (readFd == -1)
        {
            LOG_ERROR("read fall %s", t.c_str());
            perror("read");
        }
        ::read(readFd, chineseBuf,100000);
        washWordsCn(chineseBuf);
        LOG_DEBUG("wash success path:%s",t.c_str());
        loadDict(chineseBuf, mp, 100000);
        LOG_DEBUG("load success path:%s",t.c_str());
        memset(chineseBuf, 0, 100000);
        close(readFd);
    }
}
void DictProducer::storeDict(const char *filepath) {}
void DictProducer::showFiles() {
    for(auto &t:_files){
        LOG_INFO("showFiles %s",t.c_str());
    }
}
void DictProducer::showDict() {
    for(int i=0;i<20;i++){
        LOG_INFO("showDict %s  %d",_dict[i].first.c_str(),_dict[i].second);
    }
}
void DictProducer::getFiles() {}
void DictProducer::pushDict(const string &word) {
    string tempWord;
    for (int i = 0; i < word.size(); i++)
    {
        if (word[i] == ' ' && tempWord.size() && tempWord != " ")
        {

                int flag = _dict.size();
                for (int i = 0; i < _dict.size(); i++)
                {
                    if (_dict[i].first == tempWord)
                    {
                        _dict[i].second++;
                        flag = i;
                        break;
                    }
                }
                if (flag == _dict.size())
                {
                    _dict.push_back({tempWord, 1});
                }
                for (int i = 0; i < tempWord.size();)
                {
                    if (tempWord[i] & 0x80 == 0)
                    {
                        _index[tempWord.substr(i, 1)].insert(flag);
                        i++;
                    }
                    else{
                        _index[tempWord.substr(i, 3)].insert(flag);
                        i+=3;
                    }
                }
            tempWord.clear();
        }
        else if (word[i] == ' ')
        {

        }
        else
        {
            tempWord.push_back(word[i]);
        }
    }
}

// 辅助函数：将 UTF-8 字符串分割为字符（按字节块）
std::vector<std::string> split_to_chars(const std::string& s) {
    std::vector<std::string> chars;
    for (size_t i = 0; i < s.size();) {
        // 检查最高位是否为 0 (单字节 ASCII)
        if ((s[i] & 0x80) == 0) {
            chars.push_back(s.substr(i, 1));
            i++;
        } 
        // 假设多字节字符为 3 字节 (典型的中文 UTF-8)
        else if (i + 3 <= s.size()) { 
            chars.push_back(s.substr(i, 3));
            i += 3;
        } 
        else {
            // 处理字符串末尾不足一个完整字符的剩余字节
            chars.push_back(s.substr(i));
            break;
        }
    }
    return chars;
}
int minDistance(const std::string& word1_str, const std::string& word2_str) {
    // 1. 将输入字符串分割成字符序列
    std::vector<std::string> word1 = split_to_chars(word1_str);
    std::vector<std::string> word2 = split_to_chars(word2_str);

    int n = word1.size(); // 字符数
    int m = word2.size(); // 字符数
    
    // 2. 初始化 DP 表：尺寸为 (n+1) x (m+1)
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    // 3. 初始化边界条件
    for (int i = 0; i <= n; i++) dp[i][0] = i; // 删除 i 个字符
    for (int j = 0; j <= m; j++) dp[0][j] = j; // 插入 j 个字符

    // 4. 填充 DP 表
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            // 关键修正：比较的是完整的字符（即 std::string 块）
            if (word1[i] == word2[j]) { 
                // 匹配或不需要操作
                dp[i + 1][j + 1] = dp[i][j];
            } else {
                // 不匹配时：
                // 1. dp[i+1][j]：删除 word2[j] (从 word2 到 word1)
                // 2. dp[i][j+1]：插入 word1[i] (从 word1 到 word2)
                // 3. dp[i][j]：替换 word1[i] 为 word2[j]
                
                // 替换、删除、插入操作代价都为 1
                dp[i + 1][j + 1] = std::min({
                    dp[i + 1][j], // 删除/插入
                    dp[i][j + 1], // 插入/删除
                    dp[i][j]      // 替换
                }) + 1;
            }
        }
    }
    
    return dp[n][m];
}

// 注意：原函数签名使用了 string&，为了修改方便，这里改为 const string&
bool cmp(vector<int>& a,vector<int>& b) {
    // 核心逻辑:
    // 优先队列的比较器（如 std::less<T>）在返回 true 时，
    // 表示“a < b”，即 a 的优先级低于 b（b 应该在堆顶）。
    
    // 1. 比较 a[0] 和 b[0]
    if (a[0] != b[0]) {
        // 如果 a[0] < b[0]，则 a 优先级低于 b，返回 true (a < b)
        return a[0] > b[0];
    } 
    
    // 2. 如果 a[0] == b[0]，比较 a[1] 和 b[1]
    else {
        // 如果 a[1] < b[1]，则 a 优先级低于 b，返回 true (a < b)
        return a[1] > b[1];
    }
}
json DictProducer::find(string& words,int topK){
    set<int> ansSet;
    for (int i = 0; i < words.size();) {
        // 关键修正：使用括号确保先执行位与操作。
        // 如果最高位为 0，则为单字节 ASCII 字符。
        if ((words[i] & 0x80) == 0&&words[i]!='\n') { 
            // 1. 处理单字节字符（英文、数字、标点符号）
            // 确保不会超出字符串末尾
            if (i < words.size()) {
                for (auto& t : _index[words.substr(i, 1)])
                    ansSet.insert(t);
                i++; // 步进 1 字节
            } else {
                break; 
            }
        } else {
            // 2. 处理多字节字符（中文，假设为 3 字节 UTF-8）
            // 检查剩余长度是否足够 3 字节
            if (i + 3 <= words.size()) {
                for (auto& t : _index[words.substr(i, 3)])
                    ansSet.insert(t);
                i += 3; // 步进 3 字节
            } else {
                // 如果剩余字节不足 3 (可能是被截断的字符)，跳出循环
                break; 
            }
        }
    }
    //全部候选词算一遍编辑距离，然后编辑距离和词频加入到一个vector<int>中，最后面再由hash表拿到词语，返回出去
    priority_queue<vector<int>,vector<vector<int>>,decltype(cmp)*> pq(cmp);
    for(auto &t:ansSet){
        int temp=minDistance(words,_dict[t].first);
        pq.push({temp,_dict[t].second,t});
    }
    vector<string> ans;
    vector<int> temp={0,0,0};
    for(int i=0;i<topK;i++){
        if(!pq.empty())temp=pq.top();
        else continue;
        pq.pop();
        ans.push_back(_dict[temp[2]].first);
    }
    return ans;
}