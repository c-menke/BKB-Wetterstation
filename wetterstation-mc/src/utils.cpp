#include "config.h"

#ifndef __UTILS_H_INC__
#define __UTILS_H_INC__

#ifdef ENABLE_DEBUG
#define DEBUG(str) Serial.println(str)
#define DEBUG_ARGS(str, str1) Serial.println(str, str1)
#define DEBUG2(str) Serial.print(str)
#define DEBUG_WRITE(c) Serial.write(c)
#else
#define DEBUG(str)
#define DEBUG_ARGS(str, str1)
#define DEBUG2(str)
#define DEBUG_WRITE(c)
#endif

#endif