#include "SplitTool.h"
#include "mylog.h"

SplitTool* SplitTool::_ptr = nullptr;
SplitTool::SplitTool(const string& dictPath, const string& hmmPath, const string& userDictPath,
                     const string& idfPath, const string& stopWordPath)
:cutHelper(dictPath, hmmPath, userDictPath, idfPath, stopWordPath)
,simHelper(dictPath, hmmPath, idfPath, stopWordPath)
{

}

SplitTool::~SplitTool()
{

}
void SplitTool::init(Configer& conf) {
    if (_ptr == nullptr) {
        auto& cfg = conf.getConfigMap();
        _ptr = new SplitTool(
            cfg["jiebaDictPath"],
            cfg["hmmPath"],
            cfg["userDictPath"],
            cfg["idfPath"],
            cfg["jiebaStopWordPath"]
        );
    }
}
SplitTool* SplitTool::getPtr(){
    if(_ptr==nullptr){
        LOG_ERROR("SplitTool::getPtr() called before init()!");
        return nullptr;
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
    return simHelper.isEqual(a,b,c);
}
