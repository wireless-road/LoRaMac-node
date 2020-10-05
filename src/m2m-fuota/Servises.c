//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "Servises.h"
#include "LoRaWan.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "SERV:"
#include "syslog.h"


//******************************************************************************
// Pre-processor Definitions
//******************************************************************************
#define SYSTIME_REQ_TIMEOUT 60
//******************************************************************************
// Private Types
//******************************************************************************

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************
static SysTime_t LastTimeClockSynched =
{
	    .Seconds = 0,
	    .SubSeconds = 0,
};


/*
 * Indicates if the system time has been synchronized
 */
static volatile bool IsClockSynched = false;

static TimerEvent_t ClockSynchTimer;

static LmhpComplianceParams_t LmhpComplianceParams =
{
    .AdrEnabled = LORAWAN_ADR_STATE,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .StopPeripherals = NULL,
    .StartPeripherals = NULL,
};

//******************************************************************************
// Public Data
//******************************************************************************
uint32_t ServisesTaskId;

PROCESS_FUNC ServisesFunc =
{
	.Init = ServisesInit,
	.Proc = ServisesTimeProc,
	.Stop = ServisesStop,
	.IsNeedRun = ServisesIsRun,
};
//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Public Functions
//******************************************************************************

/*!
 * Function executed on Clock Synch event
 */
static void ClockSynchTimerEvent( void* context )
{
    TimerStop( &ClockSynchTimer );

    IsClockSynched = false;
}

#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection )
{
    SysTime_t Time  = SysTimeGet();

    SYSLOG_I("SysTimeUpdate. New Time = %d. timeCorrection = %d", Time.Seconds, timeCorrection);
    IsClockSynched = true;

    TimerSetValue( &ClockSynchTimer, (24 *3600 * 1000));
    TimerStart( &ClockSynchTimer );
}
#else
static void OnSysTimeUpdate( void )
{
	SysTime_t Time  = SysTimeGet();

    SYSLOG_I("SysTimeUpdate. New Time = %d", Time.Seconds);
    IsClockSynched = true;
}
#endif

void ServisesInit(void)
{
	LmHandlerPackageRegister( PACKAGE_ID_CLOCK_SYNC, NULL );
	LmHandlerPackageRegister( PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams );

	TimerInit( &ClockSynchTimer, ClockSynchTimerEvent );

	IsClockSynched = false;
}

void ServisesTimeProc(void)
{
	if (IsClockSynched == false)
	{
		if (LmhpClockSyncAppTimeReq( ) == LORAMAC_HANDLER_SUCCESS)
		{
			SYSLOG_I("SysTime Req");
			LastTimeClockSynched = SysTimeGetMcuTime();
		}
	}
}


bool ServisesIsRun(void)
{
	if ((IsClockSynched == false) && (SysTimeGetMcuTime().Seconds > (LastTimeClockSynched.Seconds + SYSTIME_REQ_TIMEOUT )))
	{
		return true;
	}
	return false;
}

void ServisesStop(void)
{

}





