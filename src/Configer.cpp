#include "Configer.h"
#include<string.h>
#include"mylog.h"
Configer::Configer(const string& path)
{
    int fd=open(path.c_str(),O_RDONLY);
    char buf[1024];
    memset(buf,0,1024);
    int err=::read(fd,buf,1024);
    if(err==-1){
        LOG_ERROR("read fall on fd:%d",fd);
        perror("read");
    }
    string key;
    string val;
    int flag=0;
    for(int i=0;i<1024;i++){
        if(buf[i]==' '){

        }
        else if(buf[i]==':'){
            flag=1;
        }
        else if(buf[i]=='\n'){
            _configMap[key]=val;
            LOG_INFO("读取配置成功 key=%s,val=%s",key.c_str(),val.c_str());
            key.clear();
            val.clear();
            flag=0;
        }
        else if(flag){
            val.push_back(buf[i]);
        }
        else{
            key.push_back(buf[i]);
        }
    }
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