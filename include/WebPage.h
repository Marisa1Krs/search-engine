#ifndef WEBPAGE_H
#define WEBPAGE_H
#pragma once

const static int TOPK_NUMBER = 20;
#include"Configer.h"
#include"SplitTool.h"
#include<string>
using std::string;
class WebPage
{
public:
    WebPage(int id,string& _title,string& _url,string _content);
    ~WebPage();
    string processDoc();
private:
    string _doc;
    int _id;
    string _title;
    string _url;
    string _content;
    SplitTool* _jieba;
   
    // void calcTopK(vector<string>& words,int k=TOPK_NUMBER,set<string>& stopWord);
    friend bool operator==(const WebPage & lhs, const WebPage & rhs);
    friend bool operator < (const WebPage & lhs, const WebPage & rhs);
};

#endif