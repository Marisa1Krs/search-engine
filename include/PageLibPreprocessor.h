#ifndef PAGELIBPREPROCESSOR_H
#define PAGELIBPREPROCESSOR_H
#pragma once
#include"SplitTool.h"
#include"WebPage.h"
#include"Configer.h"
#include<vector>
#include<map>
#include<unordered_map>
#include<string>
#include"json.hpp"
using namespace nlohmann;
using std::vector;
using std::map;
using std::unordered_map;
using std::string;


class PageLibPreprocessor
{
public:
    PageLibPreprocessor(Configer& conf);
    ~PageLibPreprocessor();
    void doProcess();
    json find(const string& str);
    PageLibPreprocessor operator=(const PageLibPreprocessor& temp)=delete;
    PageLibPreprocessor(const PageLibPreprocessor& temp)=delete;
    static void init(Configer& conf){
        if(_ptr==nullptr){
            _ptr=new PageLibPreprocessor(conf);
        }
    }
    static PageLibPreprocessor* getPtr(){
        return _ptr;
    }
private:
    SplitTool* _jieba;
    map<int,pair<int,int>> _offsetLib;
    unordered_map<string,vector<pair<int,double>>> _invertIndex;
    void readInfoFromFile();//读内容
    bool cutRedundantPages(string text,vector<uint64_t>& helper);//去重
    void buildInvertIndex();//创建倒排索引
    void storeOnDisk(int fd,string& text);
    Configer& _conf;
    map<string,int> dictAll;
    static PageLibPreprocessor* _ptr;
    void operator delete(void* temp){
        ::delete(PageLibPreprocessor*)temp;
    }
};

#endif