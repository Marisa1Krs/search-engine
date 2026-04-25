#ifndef SPLITTOOL_H
#define SPLITTOOL_H
//结巴类的封装
#pragma once
#include"cppjieba/Jieba.hpp"
#include"simhash/Simhasher.hpp"
#include"Configer.h"
using namespace cppjieba;
using namespace simhash;
class SplitTool
{
public:
    SplitTool(const string& dictPath, const string& hmmPath, const string& userDictPath,
              const string& idfPath, const string& stopWordPath);
    ~SplitTool();
    SplitTool(const SplitTool&)=delete;
    SplitTool operator =(const SplitTool&)=delete;
    static void init(Configer& conf);
    static SplitTool* getPtr();
    vector<string> cut(string& sentens);
    vector<pair<string,double>> extract(string& s,int topN);
    uint64_t make(string& s,int topN);
    uint64_t binaryToUint64(string& s);
    bool isEqual(uint64_t a,uint64_t b);
    bool isEqual(uint64_t a,uint64_t b,int c);
private:
    void operator delete(void* temp){
        ::delete temp;
    }
    static SplitTool* _ptr;
    Jieba cutHelper;
    Simhasher simHelper;
};

#endif