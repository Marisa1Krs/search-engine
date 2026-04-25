#ifndef DICTPRODUCER_H
#define DICTPRODUCER_H
#pragma once
#include"SplitTool.h"
#include"Configer.h"
#include<vector>
#include<map>
#include<string>
#include"mylog.h"
#include <dirent.h>
#include"json.hpp"
#include<allfun_of_c++.h>
using namespace std;
using namespace nlohmann;
using std::vector;
using std::map;
using std::string;
class DictProducer
{
public:
    DictProducer(const string& dir);
    DictProducer(const string& dir,SplitTool* tool);
    ~DictProducer();
    void buildEnDict();
    void buildCnDict();
    void storeDict(const char* filepath);//将词典写入文件
    json find(const string& words,int topK);
    void showFiles();
    void showDict();
    void getFiles();
    void pushDict(const string & word);
    DictProducer operator=(const DictProducer& temp)=delete;
    DictProducer(const DictProducer& temp)=delete;
    // 从配置文件初始化字典构建器的单例
    static void init(Configer& conf) {
        if (_ptr == nullptr) {
            // 必须先初始化 SplitTool（它需要配置中的 jieba 词典路径）
            SplitTool::init(conf);
            auto& cfg = conf.getConfigMap();
            string chineseDir = cfg["chineseDir"];
            // 从配置中读取停用词路径和缓冲区大小
            _stopWordsEnPath = cfg["stopWordsEnPath"];
            _stopWordsCnPath = cfg["stopWordsCnPath"];
            _englishBufSize = std::stoi(cfg["englishBufSize"]);
            _chineseBufSize = std::stoi(cfg["chineseBufSize"]);
            _stopFileBufSize = std::stoi(cfg["stopFileBufSize"]);
            _ptr = new DictProducer(chineseDir, SplitTool::getPtr());
        }
    }
    static DictProducer* getPtr(){
        return _ptr;
    }
private:
    void washWordsEn(char* words);//清洗英文文本:大写全部变成小写，其他标点符号全部变成空格
    void washWordsCn(char* words);
    void loadDict(char *words,unordered_map<string,int>& mp,size_t size);//加载字典
    vector<string> _files;
    vector<pair<string,int>> _dict;
    SplitTool* _splitTool=nullptr ;
    map<string,set<int>> _index;
    static DictProducer* _ptr;
    // 从配置文件读取的路径和缓冲区大小（在 init() 中设置）
    static string _stopWordsEnPath;
    static string _stopWordsCnPath;
    static int _englishBufSize;
    static int _chineseBufSize;
    static int _stopFileBufSize;
    void operator delete(void* temp){
        ::delete(DictProducer*)temp;
    }
};

#endif