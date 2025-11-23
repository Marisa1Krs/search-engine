#include "SplitTool.h"

SplitTool* SplitTool::_ptr = nullptr;
SplitTool::SplitTool()
:cutHelper(DICT_PATH,HMM_PATH,USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH)
,simHelper(DICT_PATH,HMM_PATH,IDF_PATH,STOP_WORD_PATH)
{

}

SplitTool::~SplitTool()
{

}
SplitTool* SplitTool::getPtr(){
    if(_ptr==nullptr){
        _ptr=new SplitTool();
    }
    return _ptr;
}
vector<string> SplitTool::cut(string& sentens){
    vector<string> help;
    cutHelper.Cut(sentens,help,1);
    return help;
}
vector<pair<string,double>> SplitTool::extract(string& s,int topN){
    vector<pair<string,double>> ans;
    simHelper.extract(s,ans,topN);
    return ans;
}
uint64_t SplitTool::make(string& s,int topN){
    uint64_t ans;
    simHelper.make(s,topN,ans);
    return ans;
}
uint64_t SplitTool::binaryToUint64(string& s){
    return simHelper.binaryStringToUint64(s);
}
bool SplitTool::isEqual(uint64_t a,uint64_t b){
    return simHelper.isEqual(a,b);//默认海明距离小于3时相等
}
bool SplitTool::isEqual(uint64_t a,uint64_t b,int c){
    return simHelper.isEqual(a,b,c);//默认海明距离小于3时相等
}