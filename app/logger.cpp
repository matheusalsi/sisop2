#include "logger.h"

void Logger::log(std::string const& str){
    logLock.lock();
    if(full){
        _log.pop_front();
    }
    else if(_log.size() == (LOG_LIMIT - 1)){
        full = true;
    }
    auto cur_time = std::chrono::system_clock::now();
    _log.push_back(std::pair<time_t, std::string>(std::chrono::system_clock::to_time_t(cur_time), str));
    logLock.unlock();
}

void Logger::log(const char* str){
    std::string _str = std::string(str);
    log(_str);
}

std::string Logger::getLogsString(){
    std::stringstream ss;
    logLock.lock();
    for(auto s : _log){
        auto time = gmtime(&s.first);
        ss << "[" << std::put_time(time, "%Y-%m-%d %H:%M:%S") << "] - " << s.second << std::endl;
    }
    logLock.unlock();
    return ss.str();
}