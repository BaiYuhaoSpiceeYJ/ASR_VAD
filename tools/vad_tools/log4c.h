#ifndef LOG4C_H
#define LOG4C_H

#include <stdio.h>
#include <string.h>
#include <time.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

enum Log4CLevel {
  LOG4C_ALL     = 0,
  LOG4C_TRACE   = 1,
  LOG4C_DEBUG   = 2,
  LOG4C_INFO    = 3,
  LOG4C_WARNING = 4,
  LOG4C_ERROR   = 5,
  LOG4C_OFF     = 6
};

static const char* LOG_NAME[] = {"ALL",
                                 "TRACE",
                                 "DEBUG",
                                 "INFO",
                                 "WARN",
                                 "ERROR",
                                 "OFF"};

#define LOG4C_LEVEL_ALL

#if defined LOG4C_LEVEL_OFF
    static int log4c_level = LOG4C_OFF;   
#elif defined LOG4C_LEVLE_ERROR
    static int log4c_level = LOG4C_ERROR;    
#elif defined LOG4C_LEVEL_WARNING
    static int log4c_level = LOG4C_WARNING;
#elif defined LOG4C_LEVEL_INFO
    static int log4c_level = LOG4C_INFO;
#elif defined LOG4C_LEVEL_DEBUG
    static int log4c_level = LOG4C_DEBUG;
#elif defined LOG4C_LEVEL_TRACE
    static int log4c_level = LOG4C_TRACE;
#elif defined LOG4C_LEVEL_ALL
    static int log4c_level = LOG4C_ALL;
#endif



static time_t log4c_t;
static struct tm * log4c_local_t;
#define LOG(level, fmt, ...) \
  if (level >= log4c_level) { \
    time (&log4c_t); \
    log4c_local_t = localtime (&log4c_t); \
    fprintf(stderr, \
            "[%5s] %d/%d/%d %d:%d:%d %s@%-15s#%d:\n===>  " fmt "\n", \
            LOG_NAME[level], \
            log4c_local_t->tm_year+1900, \
            log4c_local_t->tm_mon, \
            log4c_local_t->tm_mday, \
            log4c_local_t->tm_hour, \
            log4c_local_t->tm_min, \
            log4c_local_t->tm_sec, \
            __func__, \
            __FILENAME__, \
            __LINE__, \
            ##__VA_ARGS__); \
  }

#define LOG_ALL(fmt, ...)   LOG(LOG4C_ALL, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) LOG(LOG4C_TRACE, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) LOG(LOG4C_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(LOG4C_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(LOG4C_WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LOG4C_ERROR, fmt, ##__VA_ARGS__)

#endif

