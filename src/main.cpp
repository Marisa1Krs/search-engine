#include "Jieba.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "Simhasher.hpp"
#include"json.hpp"
#include"mylog.h"
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace simhash;
using json = nlohmann::json;
string logPath="/home/marisa/code1/search-engine/log/log.txt";

int main()
{
    mylog::init(logPath,4096,LOG_DEBUG);
    LOG_DEBUG("hello world");
    return 0;
}