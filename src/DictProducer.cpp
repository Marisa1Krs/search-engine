#include "DictProducer.h"

DictProducer::DictProducer(const string& dir)
:_files()
{
    LOG_INFO("Create DictProducer ,dir is %s",dir.c_str());
    DIR* fileDir=opendir(dir.c_str());
    if(fileDir==nullptr){
        LOG_ERROR("open filedir fall,path is %s",dir.c_str());
        perror("opendir");
    }
    struct dirent* enterDir;
    while ((enterDir = readdir(fileDir)) != NULL) {

        if (strcmp(enterDir->d_name, ".") == 0 || strcmp(enterDir->d_name, "..") == 0) {
            continue;
        }
        if(enterDir->d_type==DT_REG){
            _files.push_back(dir+"/"+string(enterDir->d_name));
        }
    }
    
}//英文
DictProducer::DictProducer(const string& dir,SplitTool* tool)
:_files()
,_splitTool(tool)
{
    LOG_INFO("Create DictProducer ,dir is %s",dir.c_str());
    DIR* fileDir=opendir(dir.c_str());
    if(fileDir==nullptr){
        LOG_ERROR("open filedir fall,path is %s",dir.c_str());
        perror("opendir");
    }
    struct dirent* enterDir;
    while ((enterDir = readdir(fileDir)) != NULL) {

        if (strcmp(enterDir->d_name, ".") == 0 || strcmp(enterDir->d_name, "..") == 0) {
            continue;
        }
        if(enterDir->d_type==DT_REG){
            _files.push_back(dir+"/"+string(enterDir->d_name));
        }
    }
}//中文

DictProducer::~DictProducer()
{

}
void DictProducer::buildEnDict(){}
void DictProducer::buildCnDict(){}
void DictProducer::storeDict(const char* filepath){}
void DictProducer::showFiles(){}
void DictProducer::showDict(){}
void DictProducer::getFiles(){}
void DictProducer::pushDict(const string & word){}