#include "Jieba.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "Simhasher.hpp"
#include"json.hpp"
using std::cout;
using std::endl;
using std::string;
using std::vector;
using namespace simhash;
using json = nlohmann::json;

const char *const DICT_PATH = "/home/marisa/code1/search-engine/dict/jieba.dict.utf8";      // 最大概率法
const char *const HMM_PATH = "/home/marisa/code1/search-engine/dict/hmm_model.utf8";        // 隐式马尔科夫
const char *const USER_DICT_PATH = "/home/marisa/code1/search-engine/dict/user.dict.utf8";  // 用户自
const char *const IDF_PATH = "/home/marisa/code1/search-engine/dict/idf.utf8";              // IDF路径
const char *const STOP_WORD_PATH = "/home/marisa/code1/search-engine/dict/stop_words.utf8"; // 停用词

int main()
{

    return 0;
}