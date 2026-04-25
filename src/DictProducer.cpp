#include "DictProducer.h"

using namespace std;

// 静态成员变量定义
string DictProducer::_stopWordsEnPath;
string DictProducer::_stopWordsCnPath;
int DictProducer::_englishBufSize = 6488671;
int DictProducer::_chineseBufSize = 100000;
int DictProducer::_stopFileBufSize = 8192;

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
    size_t len = strlen(words);
    for (int i = 0; i < len; i++)
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
    int banFd = open(_stopWordsEnPath.c_str(), O_RDONLY); // 加载停用词
    char buf[_stopFileBufSize];
    memset(buf, 0, _stopFileBufSize);
    ::read(banFd, buf, _stopFileBufSize);
    close(banFd);
    unordered_map<string, int> mp;
    string tempWord;
    for (int i = 0; i < _stopFileBufSize; i++)
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
    char* englishBuf = new char[_englishBufSize];
    memset(englishBuf, 0, _englishBufSize);
    for (auto &t : _files)
    {
        int readFd = open(t.c_str(), O_RDONLY);
        if (readFd == -1)
        {
            LOG_ERROR("read fall %s", t.c_str());
            perror("read");
        }
        ::read(readFd, englishBuf, _englishBufSize);
        washWordsEn(englishBuf);
        LOG_DEBUG("wash success path:%s",t.c_str());
        loadDict(englishBuf, mp, _englishBufSize);
        LOG_DEBUG("load success path:%s",t.c_str());
        memset(englishBuf, 0, _englishBufSize);
        close(readFd);
    }
    delete[] englishBuf;
}
void DictProducer::buildCnDict() {
    int banFd = open(_stopWordsCnPath.c_str(), O_RDONLY); // 加载停用词
    char buf[_stopFileBufSize];
    memset(buf, 0, _stopFileBufSize);
    ::read(banFd, buf, _stopFileBufSize);
    close(banFd);
    unordered_map<string, int> mp;
    string tempWord;
    for (int i = 0; i < _stopFileBufSize; i++)
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
    char* chineseBuf = new char[_chineseBufSize];
    memset(chineseBuf, 0, _chineseBufSize);
    for (auto &t : _files)
    {
        int readFd = open(t.c_str(), O_RDONLY);
        if (readFd == -1)
        {
            LOG_ERROR("read fall %s", t.c_str());
            perror("read");
        }
        ::read(readFd, chineseBuf,_chineseBufSize);
        washWordsCn(chineseBuf);
        LOG_DEBUG("wash success path:%s",t.c_str());
        loadDict(chineseBuf, mp, _chineseBufSize);
        LOG_DEBUG("load success path:%s",t.c_str());
        memset(chineseBuf, 0, _chineseBufSize);
        close(readFd);
    }
    delete[] chineseBuf;
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
    std::vector<std::string> word1 = split_to_chars(word1_str);
    std::vector<std::string> word2 = split_to_chars(word2_str);
    int n = word1.size();
    int m = word2.size();
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = 0; i <= n; i++) dp[i][0] = i;
    for (int j = 0; j <= m; j++) dp[0][j] = j;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            if (word1[i] == word2[j]) {
                dp[i + 1][j + 1] = dp[i][j];
            } else {
                dp[i + 1][j + 1] = std::min({
                    dp[i + 1][j],
                    dp[i][j + 1],
                    dp[i][j]
                }) + 1;
            }
        }
    }
    return dp[n][m];
}
bool cmp(vector<int>& a,vector<int>& b) {
    if (a[0] != b[0]) {
        return a[0] > b[0];
    } else {
        return a[1] > b[1];
    }
}
json DictProducer::find(const string& words,int topK){
    set<int> ansSet;
    for (int i = 0; i < words.size();) {
        if ((words[i] & 0x80) == 0&&words[i]!='\n') {
            if (i < words.size()) {
                for (auto& t : _index[words.substr(i, 1)])
                    ansSet.insert(t);
                i++;
            } else {
                break;
            }
        } else {
            if (i + 3 <= words.size()) {
                for (auto& t : _index[words.substr(i, 3)])
                    ansSet.insert(t);
                i += 3;
            } else {
                break;
            }
        }
    }
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