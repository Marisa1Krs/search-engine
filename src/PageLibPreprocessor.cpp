#include "PageLibPreprocessor.h"
#include <dirent.h>
#include"mylog.h"
#include"tinyxml2.h"
const int TOPNVAL=10;
const char *banPathEn1 = "/home/marisa/code1/search-engine/data/yuliao/stop_words_eng.txt";
const char *banPathCn1 = "/home/marisa/code1/search-engine/data/yuliao/stop_words_zh.txt";
using namespace tinyxml2;
PageLibPreprocessor::PageLibPreprocessor(Configer& conf)
:_conf(conf)
,_jieba(SplitTool::getPtr())
{

}

PageLibPreprocessor::~PageLibPreprocessor()
{

}
void PageLibPreprocessor::doProcess(){
    readInfoFromFile();
    buildInvertIndex();
}
void PageLibPreprocessor::readInfoFromFile(){
    string pathWebPage=_conf.getConfigMap()["webPagePath"];
    int storeFd=open(pathWebPage.c_str(),O_RDWR| O_CREAT | O_APPEND,0666);
    string dir=_conf.getConfigMap()["xmlPath"];
    vector<string> files;
    LOG_INFO("Create PageLib ,dir is %s", dir.c_str());
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
            files.push_back(temp);
            LOG_DEBUG("file is pushed back %s", temp.c_str());
        }
    }

    int docid=0;
    vector<uint64_t> simHelp;
    size_t offset=0;

    for(auto &t:files){
        XMLDocument doc;
        XMLError eResult = doc.LoadFile(t.c_str());
        if (eResult != XML_SUCCESS) {
            LOG_ERROR("XML file loaded failed: %s",doc.ErrorStr());
        }
        XMLElement* root=doc.FirstChildElement("rss");
        XMLElement* begin=root->FirstChildElement("channel");


        auto GetSafeText = [](tinyxml2::XMLElement* element) -> std::string {
            if (element == nullptr) {
                return "";
            }
            const char* text = element->GetText();
            return (text != nullptr) ? text : "";
        };
        
        for (auto it = begin->FirstChildElement("item"); it != nullptr; it = it->NextSiblingElement("item")) {
            
            // --- 获取 Title ---
            auto nodeTitle = it->FirstChildElement("title");
            string title = GetSafeText(nodeTitle);
            LOG_DEBUG("title text: %s", title.c_str());
        
            // --- 获取 Content / Description ---
            // 优先获取 description
            auto nodeDesc = it->FirstChildElement("description");
            string content = GetSafeText(nodeDesc);
        
            // 如果 description 为空，尝试获取 content
            // 【修复】这里去掉了 "string" 类型声明，直接赋值给外部变量，修复了变量遮蔽 Bug
            if (content.empty()) {
                auto nodeContent = it->FirstChildElement("content");
                content = GetSafeText(nodeContent);
            }
            if(!content.empty()&&!cutRedundantPages(content,simHelp)){
                continue;
            }//如果去重检测不通过，那么就直接进行下一轮
        
            // --- 获取 Link ---
            auto nodeUrl = it->FirstChildElement("link");
            string url = GetSafeText(nodeUrl);
            LOG_DEBUG("URL text: %s", url.c_str());
        
         
            WebPage temp(docid++, title, url, content);
            
            string text = temp.processDoc();
            
            // 只有当内容不为空时才写入，增加健壮性
            if (!content.empty()&&!text.empty()) {
                storeOnDisk(storeFd, text);
                _offsetLib[docid-1].first=offset;
                _offsetLib[docid-1].second=text.size();
                offset+=text.size()+1;
                LOG_DEBUG("text be store length: %lu", text.size()); // 打印长度比打印整个内容更安全，防止日志刷屏
            }
        }
    }
    close(storeFd);
}
bool PageLibPreprocessor::cutRedundantPages(string text,vector<uint64_t>& helper){
    uint64_t simText=_jieba->make(text,TOPNVAL);
    for(auto& t:helper){
        if(_jieba->isEqual(t,simText,3)){
            LOG_DEBUG("in same as text is pushed");
            return 0;
        }
    }
    helper.push_back(simText);
    return 1;
}//去重
void PageLibPreprocessor::buildInvertIndex(){
    int banFd = open(banPathCn1, O_RDONLY); // 加载停用词
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
                mp[tempWord] = -1;
            }
            LOG_DEBUG("word %s is banned", tempWord.c_str());
            tempWord.clear();
        }
        else
        {
            tempWord.push_back(buf[i]);
        }
    }
    string pathWebPage=_conf.getConfigMap()["webPagePath"];
    int fd = open(pathWebPage.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open file error");
        return;
    }

    vector<map<string,int>> dict;
    map<string,int> dictAll;
    string buffer; 
    for (const auto& item : _offsetLib) {
        int docId = item.first + 1;        
        off_t offset = item.second.first; 
        size_t length = item.second.second;
        buffer.resize(length);
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek error");
            continue;
        }
        ssize_t bytesRead = read(fd, &buffer[0], length);
        XMLDocument doc;
        XMLError err = doc.Parse(buffer.c_str());
        if (err != XML_SUCCESS) {
            LOG_ERROR("parse xml flase");
            continue;
        }
        XMLElement* root = doc.RootElement();
        if (!root) {
            LOG_ERROR("can not find root");
            continue;
        }
        XMLElement* contentNode = root->FirstChildElement("content");
        string tempText=contentNode->GetText();
        vector<string> help=SplitTool::getPtr()->cut(tempText);
        map<string,int> tempMap;
        for(auto &t:help){
            dictAll[t]++;
            tempMap[t]++;
        }
        dict.push_back(tempMap);
    }//第一次初始化词典


    for (const auto& item : _offsetLib) {
        int docId = item.first + 1;        
        off_t offset = item.second.first; 
        size_t length = item.second.second;
        buffer.resize(length);
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek error");
            continue;
        }
        ssize_t bytesRead = read(fd, &buffer[0], length);
        XMLDocument doc;
        XMLError err = doc.Parse(buffer.c_str());
        if (err != XML_SUCCESS) {
            LOG_ERROR("parse xml flase");
            continue;
        }
        XMLElement* root = doc.RootElement();
        if (!root) {
            LOG_ERROR("can not find root");
            continue;
        }
        XMLElement* contentNode = root->FirstChildElement("content");
        string tempText=contentNode->GetText();
        vector<string> help=SplitTool::getPtr()->cut(tempText);
        double totalDocs = static_cast<double>(dict.size());

        for (const auto &t : help) {
            // 1. 去重检查 (你的原逻辑)
            if (mp.find(t) != mp.end()) {
                continue;
            }
            // 2. 安全获取 TF (Term Frequency) - 当前文档中的频率
            // 使用 find 而不是 []，防止插入脏数据
            double tf = 0.0;
            // 假设 dict 是 vector，先取 docId 对应的 map
            const auto &docMap = dict[docId]; 
            auto itTF = docMap.find(t);
            
            if (itTF != docMap.end()) {
                tf = static_cast<double>(itTF->second);
            } else {
                // 如果当前文档里都没这个词，这逻辑就不对了，直接跳过
                continue; 
            }
        
            // 3. 安全获取 DF (Document Frequency) - 包含该词的文档总数
            double df = 0.0;
            auto itDF = dictAll.find(t);
            
            if (itDF != dictAll.end()) {
                df = static_cast<double>(itDF->second);
            } else {
                // 如果全局词典里没有这个词，设为0 (后面加1避免除零)
                df = 0.0;
            }
        
            // 4. 计算 IDF 和 权重
            // 标准公式：IDF = log2(N / (DF + 1))
            double idf = log2(totalDocs / (df + 1));
            
            // 计算最终权重
            double w = tf * idf;
            // 5. 存入倒排索引
            // _invertIndex 这里用 [] 是安全的，因为我们要写入。
            // 如果不存在则创建 vector 并 push_back
            _invertIndex[t].push_back({docId, w});
        }
    }


    LOG_INFO("build invertIndex suess");
    close(fd);
}
void PageLibPreprocessor::storeOnDisk(int fd,string& text){
    ::write(fd,text.c_str(),text.size()+1);
}