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
    json find(string& words,int topK);
    void showFiles();
    void showDict();
    void getFiles();
    void pushDict(const string & word);
    DictProducer operator=(const DictProducer& temp)=delete;
    DictProducer(const DictProducer& temp)=delete;
    // 从配置文件初始化字典构建器的单例
    static void init(Configer& conf) {
        if (_ptr == nullptr) {
            string chineseDir = conf.getConfigMap()["chineseDir"];
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
    void operator delete(void* temp){
        ::delete(DictProducer*)temp;
    }
};

#endif