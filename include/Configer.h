#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#pragma once
#include<iostream>
#include<map>
#include<set>
using std::string;
using std::map;
using std::set;
class Configer
{
public:
    Configer(const string& path);//加载配置文件
    ~Configer();    
    map<string,string>& getConfigMap();
    set<string>& getStopWord();
private:
    string _filePath;
    map<string,string> _configMap;
    set<string> _stopWord;
};

#endif