#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "logger.h"

extern bool g_exiting;
extern bool g_electionHappening;
extern bool g_foundManager;

extern Logger logger;

extern std::string g_myIP;
extern std::string g_myMACAddress;
extern std::string g_myHostname;

#endif // __GLOBALS_H__
