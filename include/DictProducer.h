#ifndef DICTPRODUCER_H
#define DICTPRODUCER_H
#pragma once
#include"SplitTool.h"
#include<vector>
#include<map>
#include<string>
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
    void storeDict(const char* filepath);
    void showFiles();
    void showDict();
    void getFiles();
    void pushDict(const string & word);
private:
    vector<string> _files;
    vector<pair<string,int>> _dict;
    SplitTool* _splitTool;
    map<string,set<int>> _index;
};

#endif