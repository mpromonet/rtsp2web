/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** logger.h
**
** -------------------------------------------------------------------------*/

#pragma once

#include <unistd.h>

typedef enum {EMERG  = 0,
                      FATAL  = 0,
                      ALERT  = 100,
                      CRIT   = 200,
                      ERROR  = 300,
                      WARN   = 400,
                      NOTICE = 500,
                      INFO   = 600,
                      DEBUG  = 700,
                      NOTSET = 800
} PriorityLevel;

#include <iostream>
extern int LogLevel;
#define LOG(__level) if (__level<=LogLevel) std::cout << "\n[" << #__level << "] " << __FILE__ << ":" << __LINE__ << " "

inline void initLogger(int verbose)
{
        switch (verbose)
        {
                case 2: LogLevel=DEBUG; break;
                case 1: LogLevel=INFO; break;
                default: LogLevel=NOTICE; break;

        }
        std::cout << "log level:" << LogLevel << std::endl;
}
