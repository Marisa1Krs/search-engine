#ifndef PAGELIBPREPROCESSOR_H
#define PAGELIBPREPROCESSOR_H
#pragma once
#include"SplitTool.h"
#include"WebPage.h"
#include"Configer.h"
#include<vector>
#include<unordered_map>
#include<string>
using std::vector;
using std::unordered_map;
using std::string;
class PageLibPreprocessor
{
public:
    PageLibPreprocessor(Configer& conf);
    ~PageLibPreprocessor();
    void doProcess();

private:
    SplitTool* _jieba;
    unordered_map<int,pair<int,int>> _offsetLib;
    unordered_map<string,vector<pair<int,double>>> _invertIndex;
    void readInfoFromFile();//读内容
    bool cutRedundantPages(string text,vector<uint64_t>& helper);//去重
    void buildInvertIndex(string& text,unordered_map<string,int>& baner,int docid);//创建倒排索引
    void storeOnDisk(int fd,string& text);
    Configer& _conf;
};

#endif