#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <deque>
#include <string>
#include <utility>
#include <iomanip>
#include <sstream>
#include <mutex>

#define LOG_LIMIT 4

class Logger{

    bool full;
    std::deque<std::pair<time_t, std::string>> _log;
    std::mutex logLock;

    public:
    void log(std::string& str);
    void log(const char* str);
    std::string getLogsString();

};


#endif