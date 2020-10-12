//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "Servises.h"
#include "LoRaWan.h"
#include "config.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "SERV:"
#include "syslog.h"


//******************************************************************************
// Pre-processor Definitions
//******************************************************************************
#define DEFAULT_SYSTIME_REQ_TIMEOUT 60 // Пауза между запросами
#define DEFAULT_SYSTIME_REQ_PERIOD	24*3600 // Каждые 24 часа
//******************************************************************************
// Private Types
//******************************************************************************

typedef struct
{
	uint32_t SysTimeReqPeriod;
	uint32_t SysTimeReqTimeout;
} __attribute__((__packed__ )) sServConf;

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

static TimerEvent_t ClockSyncTimer;

static LmhpComplianceParams_t LmhpComplianceParams =
{
    .AdrEnabled = LORAWAN_ADR_STATE,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .StopPeripherals = NULL,
    .StartPeripherals = NULL,
};

static sServConf ServConf =
{
	.SysTimeReqPeriod = DEFAULT_SYSTIME_REQ_PERIOD,
	.SysTimeReqTimeout = DEFAULT_SYSTIME_REQ_TIMEOUT,
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
    TimerStop( &ClockSyncTimer );

    IsClockSynched = false;
}

#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection )
{
    SysTime_t Time  = SysTimeGet();

    SYSLOG_I("SysTimeUpdate. New Time = %d. timeCorrection = %d", Time.Seconds, timeCorrection);
    IsClockSynched = true;

    TimerSetValue( &ClockSyncTimer, (ServConf.SysTimeReqPeriod * 1000));
    TimerStart( &ClockSyncTimer );
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
	uint32_t ConfIndex = 0;
	RESULT_CONF ConfResult;
	int ConfLen;

	LmHandlerPackageRegister( PACKAGE_ID_CLOCK_SYNC, NULL );
	LmHandlerPackageRegister( PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams );

	// Загружаем настройки
	ConfResult = ConfigPartOpen(ID_CONF_SERV, &ConfIndex); // Пытаемся открыть
	SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	if (ConfResult == CONF_NOT)
	{
		ConfResult = ConfigPartCreate(ID_CONF_SERV, 256); // Если не получилось то создаем
		SYSLOG_I("CreateConf. ConfResult=%d", ConfResult, ConfIndex);
		ConfResult = ConfigPartOpen(ID_CONF_SERV, &ConfIndex); // Пытаемся открыть
		SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	}
	if (ConfResult == CONF_OK)
	{
		ConfLen = ConfigPartRead(ConfIndex, sizeof(sServConf), &ServConf);
		if (ConfLen != sizeof(sServConf))
		{
			ConfigPartWrite(ConfIndex, sizeof(sServConf), &ServConf);
			ConfigPartRead(ConfIndex, sizeof(sServConf), &ServConf);
		}
	}


	TimerInit( &ClockSyncTimer, ClockSynchTimerEvent );

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
	if ((IsClockSynched == false) && (SysTimeGetMcuTime().Seconds > (LastTimeClockSynched.Seconds + ServConf.SysTimeReqTimeout )))
	{
		return true;
	}
	return false;
}

void ServisesStop(void)
{

}





