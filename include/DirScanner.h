#ifndef DIRSCANNER_H
#define DIRSCANNER_H
#pragma once
#include<vector>
#include<string>
using std::vector;
using std::string;
class DirScanner
{
public:
    DirScanner();
    ~DirScanner();
    vector<string>& getFiles();
    void scanDir(const string& path);
private:
    vector<string> _files;
};

#endif