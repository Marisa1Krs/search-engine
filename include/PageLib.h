#ifndef PAGELIB_H
#define PAGELIB_H
#pragma once
#include"Configer.h"
#include"DirScanner.h"
#include"FileProcessor.h"
#include<vector>
#include<map>
using std::map;
using std::vector;
using std::pair;
class PageLib
{
public:
    PageLib(Configer& configer,DirScanner& dirScanner,FileProcessor& fileProcessor);
    ~PageLib();
    void create();
    void store();
private:
    DirScanner& _scanner;
    vector<string> _files;
    map<int,pair<int,int>> _offsetLib;//存放每篇文档在网页库的位置信息
};

#endif