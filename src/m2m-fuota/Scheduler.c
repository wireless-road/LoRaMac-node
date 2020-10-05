//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "Scheduler.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "TASK:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Private Types
//******************************************************************************
typedef struct
{
	char *Name;
	PROCESS_FUNC *Func;
} PROCESS;
//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************
static PROCESS Proc[MAX_PROCESS_COUNT];

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************
static void InitAllProces(void)
{
	for (int i = 0; i < MAX_PROCESS_COUNT; i++)
	{
		if ((Proc[i].Func)&&(Proc[i].Func->Init))
		{
			Proc[i].Func->Init();
			SYSLOG_I("Process %s is init", Proc[i].Name);
		}
	}
}

static void ClearAllProces(void)
{
	for (int i = 0; i < MAX_PROCESS_COUNT; i++)
	{
		Proc[i].Name = NULL;
		Proc[i].Func = NULL;
	}
}
//******************************************************************************
// Public Functions
//******************************************************************************
void SchedulerReset(void)
{
	ClearAllProces();
}


void SchedulerStart(void)
{
	InitAllProces();
}

void SchedulerProc(void)
{
	for (int i = 0; i < MAX_PROCESS_COUNT; i++)
	{
		if ((Proc[i].Func) && (Proc[i].Func->Proc) && (Proc[i].Func->IsNeedRun() == true))
		{
			Proc[i].Func->Proc();
		}
	}
}

PROCESS_RESULT ProcessInit(const char *Name, PROCESS_FUNC *Func, uint32_t *NrRecord)
{
	if((!Name) || (!Func))
	{
		return PROCESS_ERROR;
	}
	for (int i = 0; i < MAX_PROCESS_COUNT; i++)
	{
		if (Proc[i].Func == NULL)
		{
			Proc[i].Name = (char*)Name;
			Proc[i].Func = Func;
			if (NrRecord) *NrRecord = i;
			return PROCESS_OK;
		}
	}
	return PROCESS_ERROR;
}




