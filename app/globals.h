#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "logger.h"

extern bool g_exiting;
extern bool g_electionHappening;
extern time_t g_lastElectionResponse;
extern bool g_foundManager;

extern Logger logger;
extern Logger electionLogger;

extern std::string g_myIP;
extern std::string g_myMACAddress;
extern std::string g_myHostname;

extern bool g_isManager;

#endif // __GLOBALS_H__
