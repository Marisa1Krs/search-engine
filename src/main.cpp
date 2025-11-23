#include "Jieba.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "Simhasher.hpp"
#include"json.hpp"
#include"mylog.h"
#include"DictProducer.h"
#include"Configer.h"
#include"PageLibPreprocessor.h"
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace simhash;
using json = nlohmann::json;
string logPath="/home/marisa/code1/search-engine/log/log.txt";

int main()
{
    mylog::init(logPath,4096,LOG_INFO);
    // string cnt="/home/marisa/code1/search-engine/data/yuliao/chinese";
    // DictProducer temp(cnt,SplitTool::getPtr());
    // temp.buildCnDict();
    // sleep(5);
    string confPath="/home/marisa/code1/search-engine/config/serch.conf";
    Configer temp(confPath);
    PageLibPreprocessor pageLib(temp);
    pageLib.doProcess();
    sleep(10);
    return 0;
}