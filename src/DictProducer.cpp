#include "DictProducer.h"
const char *banPathEn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_eng.txt";
const char *banPathCn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_zh.txt";
// 后续改成配置文件
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
        LOG_INFO("wash success path:%s",t.c_str());
        loadDict(englishBuf, mp ,6488666 + 5);
        LOG_INFO("load success path:%s",t.c_str());
        memset(englishBuf, 0, 6488666 + 5);
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
        LOG_INFO("wash success path:%s",t.c_str());
        loadDict(chineseBuf, mp, 100000);
        LOG_INFO("load success path:%s",t.c_str());
        memset(chineseBuf, 0, 100000);
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