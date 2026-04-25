#include "PageLibPreprocessor.h"
#include <dirent.h>
#include"mylog.h"
#include"tinyxml2.h"
PageLibPreprocessor* PageLibPreprocessor::_ptr=nullptr;
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
    int storeFd=open(pathWebPage.c_str(),O_RDWR| O_CREAT | O_TRUNC,0666);
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
    int topNVal = std::stoi(_conf.getConfigMap()["topNVal"]);
    uint64_t simText=_jieba->make(text,topNVal);
    for(auto& t:helper){
        if(_jieba->isEqual(t,simText,3)){
            LOG_DEBUG("in same as text is pushed");
            return 0;
        }
    }
    helper.push_back(simText);
    return 1;
}//去重
// 解码常见的 HTML 实体
std::string decodeHtmlEntities(const std::string& input) {
    std::string result;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '&') {
            size_t semicolon = input.find(';', i);
            if (semicolon != std::string::npos && semicolon - i <= 10) {
                std::string entity = input.substr(i, semicolon - i + 1);
                if (entity == "&" "lt;")    { result += '<'; i = semicolon + 1; continue; }
                if (entity == "&" "gt;")    { result += '>'; i = semicolon + 1; continue; }
                if (entity == "&" "amp;")   { result += '&'; i = semicolon + 1; continue; }
                if (entity == "&" "quot;")  { result += '"'; i = semicolon + 1; continue; }
                if (entity == "&" "apos;")  { result += '\''; i = semicolon + 1; continue; }
                if (entity == "&" "nbsp;")  { result += ' '; i = semicolon + 1; continue; }
                // 数字实体（如 &#160; &#x00A0;）跳过
                if (entity.size() > 3 && entity[1] == '#') {
                    i = semicolon + 1;
                    continue;
                }
            }
        }
        result += input[i];
        ++i;
    }
    return result;
}

// 去除 HTML 标签：移除所有 <...> 标签及其内部属性，保留纯文本内容
// 同时解码 HTML 实体
std::string stripHtml(const std::string& input) {
    std::string result;
    bool inTag = false;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '<') {
            inTag = true;
            continue;
        }
        if (input[i] == '>') {
            inTag = false;
            // 标签结束后添加一个空格，防止标签前后文字粘连
            // 但仅在结果末尾不是空格时添加
            if (!result.empty() && result.back() != ' ') {
                result += ' ';
            }
            continue;
        }
        if (!inTag) {
            result += input[i];
        }
    }
    return decodeHtmlEntities(result);
}
void PageLibPreprocessor::buildInvertIndex(){
    int stopFileBufSize = std::stoi(_conf.getConfigMap()["stopFileBufSize"]);
    string stopWordsCnPath = _conf.getConfigMap()["stopWordsCnPath"];
    int banFd = open(stopWordsCnPath.c_str(), O_RDONLY); // 加载停用词
    char* buf = new char[stopFileBufSize];
    memset(buf, 0, stopFileBufSize);
    ::read(banFd, buf, stopFileBufSize);
    close(banFd);
    unordered_map<string, int> mp;
    string tempWord;
    for (int i = 0; i < stopFileBufSize; i++)
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
    delete[] buf;
    string pathWebPage=_conf.getConfigMap()["webPagePath"];
    int fd = open(pathWebPage.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open file error");
        return;
    }

    // 【Bug 修复】使用 map<int,map<string,int>> 以 docId 为 key，
    // 避免因部分文档解析失败跳过 push_back 导致 vector 索引与 docId 错位
    map<int,map<string,int>> dict;
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
            LOG_ERROR("parse xml flase, docId=%d", docId);
            continue;
        }
        XMLElement* root = doc.RootElement();
        if (!root) {
            LOG_ERROR("can not find root, docId=%d", docId);
            continue;
        }
        XMLElement* contentNode = root->FirstChildElement("content");
        if (!contentNode) {
            LOG_ERROR("no <content> element, docId=%d", docId);
            continue;
        }
        const char* contentText = contentNode->GetText();
        if (!contentText) {
            LOG_ERROR("content text is null, docId=%d", docId);
            continue;
        }
        string tempText = contentText;
        tempText = stripHtml(tempText);
        // DEBUG: 打印前5个文档的清洗后文本和分词结果
        vector<string> help=SplitTool::getPtr()->cut(tempText);
        map<string,int> tempMap;
        for(auto &t:help){
            dictAll[t]++;
            tempMap[t]++;
        }
        dict[docId] = tempMap;  // 以 docId 为 key 存储，不依赖 push_back 顺序
    }//第一次初始化词典：以 docId 为 key 存储词频


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
        if (bytesRead != (ssize_t)length) {
            LOG_WARN("read bytes mismatch: expect %zu, got %zd, docId=%d", length, bytesRead, docId);
        }
        XMLDocument doc;
        XMLError err = doc.Parse(buffer.c_str());
        if (err != XML_SUCCESS) {
            LOG_ERROR("parse xml false, docId=%d, buffer[0..20]=%.*s", docId, 20, buffer.c_str());
            continue;
        }
        XMLElement* root = doc.RootElement();
        if (!root) {
            LOG_ERROR("can not find root, docId=%d", docId);
            continue;
        }
        XMLElement* contentNode = root->FirstChildElement("content");
        if (!contentNode) {
            // 尝试找 description? 不，应该就是 content
            LOG_ERROR("no <content> element found, docId=%d, first child=%s",
                docId, root->FirstChildElement() ? root->FirstChildElement()->Name() : "null");
            continue;
        }
        const char* contentText = contentNode->GetText();
        if (!contentText) {
            LOG_ERROR("content text is null, docId=%d", docId);
            continue;
        }
        string tempText = contentText;
        tempText = stripHtml(tempText);
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
            // 【Bug 修复】dict 现在是 map<int,map<string,int>>，直接用 docId 访问
            // 不再依赖 vector 索引与 docId 的对齐关系
            auto docIt = dict.find(docId);
            if (docIt == dict.end()) {
                // 如果该文档在第一遍就失败了（没有词频记录），跳过
                continue;
            }
            const auto &docMap = docIt->second;
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


    LOG_INFO("build invertIndex success, total unique words=%zu, total docs=%zu, _offsetLib size=%zu",
        _invertIndex.size(), dict.size(), _offsetLib.size());
    // 检查特定词是否在索引中
    vector<string> checkWords = {"癌症", "慢性病", "慢性", "病", "天津"};
    for (auto& w : checkWords) {
        auto it = _invertIndex.find(w);
        if (it != _invertIndex.end()) {
            LOG_INFO("CHECK INDEX: word='%s' FOUND with %zu entries", w.c_str(), it->second.size());
        } else {
            LOG_WARN("CHECK INDEX: word='%s' NOT FOUND!", w.c_str());
        }
    }
    LOG_INFO("build invertindex suessful");
    close(fd);
}
double abs1(double x){
    if(x<0)return -x;
    return x;
}
json PageLibPreprocessor::find(const string& str){
    string temp=str;
    json ans = json::array();
    string pathWebPage=_conf.getConfigMap()["webPagePath"];
    int fd = open(pathWebPage.c_str(), O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("cannot open webpage file: %s", pathWebPage.c_str());
        return ans;
    }
    
    LOG_INFO("FIND: query='%s'", str.c_str());
    auto weights=SplitTool::getPtr()->extract(temp,5);
    
    LOG_INFO("FIND: extract returned %zu keywords", weights.size());
    for (size_t i = 0; i < weights.size(); i++) {
        LOG_INFO("FIND: keyword[%zu] = '%s', weight=%f", i, weights[i].first.c_str(), weights[i].second);
    }
    
    double df = 0.0;
    vector<double> simVector;
    for(auto &item:weights){
        auto t=item.first;
        double w = item.second ;
        simVector.push_back(w);
        
        // 检查倒排索引中是否有该词
        auto it = _invertIndex.find(t);
        if (it == _invertIndex.end()) {
            LOG_WARN("FIND: word '%s' NOT FOUND in _invertIndex!", t.c_str());
        } else {
            LOG_INFO("FIND: word '%s' found in _invertIndex with %zu entries",
                t.c_str(), it->second.size());
            // 打印前几个docId
            for (size_t k = 0; k < it->second.size() && k < 3; k++) {
                LOG_INFO("FIND:   entry[%zu]: docId=%d, weight=%f",
                    k, it->second[k].first, it->second[k].second);
            }
        }
    }
    set<int> set_intersection_result;
    set<int> set1;
    set<int> set2;


    if(weights.size())
    for(auto &t:_invertIndex[weights[0].first]){
        set1.insert(t.first);
    }
    else {
        LOG_WARN("FIND: no keywords extracted, returning empty");
        close(fd);
        return ans;
    }
    
    LOG_INFO("FIND: initial set1 size=%zu (from keyword '%s')", set1.size(), weights[0].first.c_str());

    for(int i=1;i<weights.size();i++){
    for(auto &t:_invertIndex[weights[i].first]){
        set2.insert(t.first);
    }
    LOG_INFO("FIND: keyword[%d]='%s' has %zu docs", i, weights[i].first.c_str(), set2.size());
    // 使用 std::set_intersection 取交集
    set_intersection(
        set1.begin(), set1.end(),
        set2.begin(), set2.end(),
        // 使用 std::inserter 将结果插入到新的 set 中
        std::inserter(set_intersection_result, set_intersection_result.begin())
    );
    LOG_INFO("FIND: intersection size=%zu", set_intersection_result.size());
    set2.clear();
    set1=set_intersection_result;
    set_intersection_result.clear();
 }


  
    vector<pair<double,int>> helper;
    //取a向量的模
    double moda=0;
    for(auto &t:simVector){
        moda+=t*t;
    }
    moda=sqrt(moda);
    
    for(auto &docid:set1){
        vector<double> simVectorb;
    for(auto &item:weights){
        for(auto &t:_invertIndex[item.first]){
            if(t.first==docid){
                simVectorb.push_back(t.second);
            }
        }    
     }
     double modb=0;
     double mutlab=0;//a和b的点积
     for(auto &t:simVectorb){
         modb+=t*t;
     }
        modb=sqrt(modb);
     for(int i=0;i<simVector.size();i++){
        mutlab+=simVector[i]*simVectorb[i];
     }
      // 【Bug 2 修复】余弦相似度分母应是模的乘积，而非加法
      if (moda == 0.0 || modb == 0.0) {
          LOG_DEBUG("FIND: skip docId=%d because moda=%f or modb=%f", docid, moda, modb);
          continue;
      }
      helper.push_back({mutlab/(moda*modb),docid});
    }
      // 【Bug 3 修复】按余弦相似度降序排序（相似度越大越好）
      sort(helper.begin(),helper.end(),[](const pair<double,int>& a,const pair<double,int>& b){
          return a.first > b.first;
      });
      LOG_INFO("FIND: after cosine similarity, helper size=%zu", helper.size());
      if (helper.size() > 0) {
          LOG_INFO("FIND: top result: docId=%d, similarity=%f", helper[0].second, helper[0].first);
      }
      string buffer;
      for(int i=0;i<static_cast<int>(helper.size());i++){
        int docId = helper[i].second ;
        off_t offset = _offsetLib[docId-1].first;
        size_t length =_offsetLib[docId-1].second;
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
        XMLElement* root=doc.FirstChildElement("doc");
        XMLElement* title=root->FirstChildElement("title");
        XMLElement* url=root->FirstChildElement("url");
        XMLElement* content=root->FirstChildElement("content");
        json docJson;
        docJson["title"]=title->GetText();
        docJson["url"]=url->GetText();
        docJson["content"]=content->GetText();
        ans.push_back(docJson);
      }
    return ans;
}
void PageLibPreprocessor::storeOnDisk(int fd,string& text){
    ::write(fd,text.c_str(),text.size()+1);
}