//******************************************************************************
//
//******************************************************************************

#ifndef _SYSLOG_H
#define _SYSLOG_H

//******************************************************************************
// Included Files
//******************************************************************************
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "xprintf.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************
#ifndef MAX_LOG_LEVEL_NOT
#define MAX_LOG_LEVEL_NOT 		0
#endif

#ifndef MAX_LOG_LEVEL_ERROR
#define MAX_LOG_LEVEL_ERROR 	1
#endif

#ifndef MAX_LOG_LEVEL_WARNING
#define MAX_LOG_LEVEL_WARNING 	2
#endif

#ifndef MAX_LOG_LEVEL_INFO
#define MAX_LOG_LEVEL_INFO 		3
#endif

#ifndef MAX_LOG_LEVEL_DEBUG
#define MAX_LOG_LEVEL_DEBUG		4
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL    			MAX_LOG_LEVEL_NOT
#endif

#ifndef LOG_MODULE
#define LOG_MODULE    			"DEF:"
#endif

#ifndef PRODUCTION

#define SYSLOG_INIT(func) xdev_out(func)

#if (LOG_LEVEL >= MAX_LOG_LEVEL_ERROR)
#define SYSLOG_E(FORMAT, ...) do {xputs("E:"); xputs(LOG_MODULE); xprintf(FORMAT, ##__VA_ARGS__); xputc('\n'); } while (0)
#define SYSDUMP_E(NAME, BUFFER, LENGTH) \
	do {								\
		xputs("E:"); 					\
		xputs(LOG_MODULE); 				\
		xputs(NAME); 					\
		wsysdump(BUFFER, LENGTH);		\
		xputc('\n'); 					\
	} while (0)
#else
#define SYSLOG_E(FORMAT, ...)
#define SYSDUMP_E(NAME, BUFFER, LENGTH)
#endif


#if (LOG_LEVEL >= MAX_LOG_LEVEL_WARNING)
#define SYSLOG_W(FORMAT, ...) do {xputs("W:"); xputs(LOG_MODULE); xprintf(FORMAT, ##__VA_ARGS__); xputc('\n'); } while (0)
#define SYSDUMP_W(NAME, BUFFER, LENGTH) \
	do {								\
		xputs("W:"); 					\
		xputs(LOG_MODULE); 				\
		xputs(NAME); 					\
		wsysdump(BUFFER, LENGTH);		\
		xputc('\n'); 					\
	} while (0)
#else
#define SYSLOG_W(FORMAT, ...)
#define SYSDUMP_W(NAME, BUFFER, LENGTH)
#endif

#if (LOG_LEVEL >= MAX_LOG_LEVEL_INFO)
#define SYSLOG_I(FORMAT, ...) do {xputs("I:"); xputs(LOG_MODULE); xprintf(FORMAT, ##__VA_ARGS__); xputc('\n'); } while (0)
#define SYSDUMP_I(NAME, BUFFER, LENGTH) \
	do {								\
		xputs("I:"); 					\
		xputs(LOG_MODULE); 				\
		xputs(NAME); 					\
		wsysdump(BUFFER, LENGTH);		\
		xputc('\n'); 					\
	} while (0)

#else
#define SYSLOG_I(FORMAT, ...)
#define SYSDUMP_I(NAME, BUFFER, LENGTH)
#endif

#if (LOG_LEVEL >= MAX_LOG_LEVEL_DEBUG)
#define SYSLOG_D(FORMAT, ...) do {xputs("D:"); xputs(LOG_MODULE); xprintf(FORMAT, ##__VA_ARGS__); xputc('\n'); } while (0)
#define SYSDUMP_D(NAME, BUFFER, LENGTH) \
	do {								\
		xputs("D:"); 					\
		xputs(LOG_MODULE); 				\
		xputs(NAME); 					\
		wsysdump(BUFFER, LENGTH);		\
		xputc('\n'); 					\
	} while (0)
#else
#define SYSLOG_D(FORMAT, ...)
#define SYSDUMP_D(NAME, BUFFER, LENGTH)
#endif

#else
	#define SYSLOG_INIT( )
    #define SYSLOG_E(...)
	#define SYSLOG_W(...)
	#define SYSLOG_I(...)
	#define SYSLOG_D(...)
#endif


//******************************************************************************
// Public Types
//******************************************************************************

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

//******************************************************************************
// Inline Functions
//******************************************************************************

//******************************************************************************
// Public Function Prototypes
//******************************************************************************

void wsysdump(const void* buff, size_t len);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */

