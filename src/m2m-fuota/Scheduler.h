//******************************************************************************
//
//******************************************************************************

#ifndef __SCHEDULER_H
#define __SCHEDULER_H

//******************************************************************************
// Included Files
//******************************************************************************

#include <stdint.h>
#include <stdbool.h>

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define MAX_PROCESS_COUNT 16
//******************************************************************************
// Public Types
//******************************************************************************

typedef struct PROCESS_FUNC_S
{
	void (*Init)(void);
	void (*Proc)(void);
	void (*Stop)(void);
	bool (*IsNeedRun)(void);
} PROCESS_FUNC;

typedef enum PROCESS_RESULT_E
{
	PROCESS_OK,
	PROCESS_ERROR,
} PROCESS_RESULT;

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

void SchedulerProc(void);
void SchedulerStart(void);
void SchedulerReset(void);

PROCESS_RESULT ProcessInit(const char *Name, PROCESS_FUNC *Func, uint32_t *NrRecord);
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __LORAWAN_H */
