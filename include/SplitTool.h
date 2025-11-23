#ifndef SPLITTOOL_H
#define SPLITTOOL_H
//结巴类的封装
#pragma once
#include"cppjieba/Jieba.hpp"
#include"simhash/Simhasher.hpp"
using namespace cppjieba;
using namespace simhash;
const char *const DICT_PATH = "/home/marisa/code1/search-engine/dict/jieba.dict.utf8";      // 最大概率法
const char *const HMM_PATH = "/home/marisa/code1/search-engine/dict/hmm_model.utf8";        // 隐式马尔科夫
const char *const USER_DICT_PATH = "/home/marisa/code1/search-engine/dict/user.dict.utf8";  // 用户自
const char *const IDF_PATH = "/home/marisa/code1/search-engine/dict/idf.utf8";              // IDF路径
const char *const STOP_WORD_PATH = "/home/marisa/code1/search-engine/dict/stop_words.utf8"; // 停用词
class SplitTool
{
public:
    SplitTool();
    ~SplitTool();
    SplitTool(const SplitTool&)=delete;  
    SplitTool operator =(const SplitTool&)=delete;
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