#include "Configer.h"
#include<string.h>
#include"mylog.h"
#include<fstream>
Configer::Configer(const string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        LOG_ERROR("cannot open config file: %s", path.c_str());
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        // 跳过空行和注释行（# 开头）
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // 查找冒号分隔符
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            LOG_WARN("invalid config line (no colon): %s", line.c_str());
            continue;
        }
        std::string key = line.substr(0, colonPos);
        std::string val = line.substr(colonPos + 1);
        // 去掉行尾可能存在的 # 注释（行内注释）
        size_t commentPos = val.find('#');
        if (commentPos != std::string::npos) {
            val = val.substr(0, commentPos);
        }
        // 去掉首尾空白
        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) { s.clear(); return; }
            size_t end = s.find_last_not_of(" \t\r\n");
            s = s.substr(start, end - start + 1);
        };
        trim(key);
        trim(val);
        if (key.empty()) continue;
        _configMap[key] = val;
        LOG_INFO("读取配置成功 key=%s,val=%s", key.c_str(), val.c_str());
    }
    ifs.close();
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