#include "PageLibPreprocessor.h"
#include <dirent.h>
#include"mylog.h"
#include"tinyxml2.h"
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
            LOG_INFO("title text: %s", title.c_str());
        
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
        
            // --- 获取 Link ---
            auto nodeUrl = it->FirstChildElement("link");
            string url = GetSafeText(nodeUrl);
            LOG_INFO("URL text: %s", url.c_str());
        
         
            WebPage temp(docid++, title, url, content);
            
            string text = temp.processDoc();
            
            // 只有当内容不为空时才写入，增加健壮性
            if (!text.empty()) {
                storeOnDisk(storeFd, text);
                LOG_DEBUG("text be store length: %lu", text.size()); // 打印长度比打印整个内容更安全，防止日志刷屏
            }
        }
    }
    
}
void PageLibPreprocessor::cutRedundantPages(){

}//去重
void PageLibPreprocessor::buildInvertIndex(){

}
void PageLibPreprocessor::storeOnDisk(int fd,string& text){
    ::write(fd,text.c_str(),text.size()+1);
}