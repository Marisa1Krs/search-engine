#include "PageLibPreprocessor.h"
#include <dirent.h>
#include"mylog.h"
#include"tinyxml2.h"
const int TOPNVAL=10;
const char *banPathEn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_eng.txt";
const char *banPathCn = "/home/marisa/code1/search-engine/data/yuliao/stop_words_zh.txt";
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
                LOG_INFO("text be store length: %lu", text.size()); // 打印长度比打印整个内容更安全，防止日志刷屏
            }
        }
    }
    cout<<1;
    
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
void PageLibPreprocessor::buildInvertIndex(string& text,unordered_map<string,int>& baner,int docid){
    vector<string> temp=SplitTool::getPtr()->cut(text);
    for(auto &t:temp){
        
    }
}
void PageLibPreprocessor::storeOnDisk(int fd,string& text){
    ::write(fd,text.c_str(),text.size()+1);
}