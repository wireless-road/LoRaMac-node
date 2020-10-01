//******************************************************************************
//
//******************************************************************************

#ifndef __SERVISES_H
#define __SERVISES_H

//******************************************************************************
// Included Files
//******************************************************************************
#include <stdbool.h>

#include "LmhpClockSync.h"
#include "LmhpCompliance.h"
#include "Scheduler.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Public Types
//******************************************************************************

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

extern uint32_t ServisesTaskId;
extern PROCESS_FUNC ServisesFunc;

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

#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection );
#else
void OnSysTimeUpdate( void );
#endif

void ServisesInit(void);

void ServisesTimeProc(void);

bool ServisesIsRun(void);

void ServisesStop(void);

  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __SERVISES_H */
