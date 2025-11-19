#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H
#pragma once
#include<string>
using std::string;
class FileProcessor
{
public:
    FileProcessor();
    FileProcessor(string titleFeature);
    ~FileProcessor();
    string process(const string& file);
private:
    string _titleFeature;
};

#endif