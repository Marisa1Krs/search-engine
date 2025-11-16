#include "SplitTool.h"
SplitTool* SplitTool::_ptr = nullptr;
SplitTool::SplitTool()
:cutHelper(DICT_PATH,HMM_PATH,USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH)
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
}