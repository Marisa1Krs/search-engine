#include "Configer.h"

Configer::Configer(const string& path)
{

}

Configer::~Configer()
{

}
map<string,string>& Configer::getConfigMap(){
    return _configMap;
}
set<string>& Configer::getStopWord(){
    return _stopWord;
}