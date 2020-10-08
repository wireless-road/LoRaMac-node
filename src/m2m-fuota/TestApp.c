//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include "TestApp.h"

#include "LoRaWan.h"
#include "board.h"
#include "config.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APP:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

/*!
 * Defines the application data transmission duty cycle. 60s, value in [ms].
 */
#define DEFAULT_APP_TX_DUTYCYCLE                    60

/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        5000

//******************************************************************************
// Private Types
//******************************************************************************
typedef struct
{
	uint32_t SendPeriod;
	uint8_t Port;
} __attribute__((__packed__ )) sAppConf;

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t TxTimer;

static volatile uint8_t IsTxFramePending = 0;

static sAppConf Conf =
{
	.SendPeriod = DEFAULT_APP_TX_DUTYCYCLE,
	.Port = 3,
};

//******************************************************************************
// Public Data
//******************************************************************************

uint32_t AppTaskId;
PROCESS_FUNC AppFunc =
{
	.Init = AppInit,
	.Proc = AppTimeProc,
	.Stop = AppStop,
	.IsNeedRun = AppIsRun,
};

//******************************************************************************
// Private Functions
//******************************************************************************
/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent( void* context )
{
    TimerStop( &TxTimer );

    IsTxFramePending = 1;

    // Schedule next transmission
    TimerSetValue( &TxTimer, (Conf.SendPeriod * 1000 ) + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
    TimerStart( &TxTimer );
}


static bool AppSend(void)
{
	uint8_t Data[4];

	Data[0] = randr( 0, 255 );
	Data[1] = randr( 0, 255 );
	Data[2] = randr( 0, 255 );
	Data[3] = randr( 0, 255 );
	return LoRaWanSend(Conf.Port, 4, Data);
}

//******************************************************************************
// Public Functions
//******************************************************************************

void AppInit(void)
{
	uint32_t ConfIndex = 0;
	RESULT_CONF ConfResult;
	int ConfLen;

	// Загружаем настройки
	ConfResult = ConfigPartOpen(ID_CONF_APP, &ConfIndex); // Пытаемся открыть
	SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	if (ConfResult == CONF_NOT)
	{
		ConfResult = ConfigPartCreate(ID_CONF_APP, 256); // Если не получилось то создаем
		SYSLOG_I("CreateConf. ConfResult=%d", ConfResult, ConfIndex);
		ConfResult = ConfigPartOpen(ID_CONF_APP, &ConfIndex); // Пытаемся открыть
		SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	}
	if (ConfResult == CONF_OK)
	{
		ConfLen = ConfigPartRead(ConfIndex, sizeof(sAppConf), &Conf);
		if (ConfLen != sizeof(sAppConf))
		{
			ConfigPartWrite(ConfIndex, sizeof(sAppConf), &Conf);
			ConfigPartRead(ConfIndex, sizeof(sAppConf), &Conf);
		}
	}

    // Schedule 1st packet transmission
    TimerInit( &TxTimer, OnTxTimerEvent );
    TimerSetValue( &TxTimer, (Conf.SendPeriod * 1000) + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
    OnTxTimerEvent( NULL );
}

void AppTimeProc(void)
{
	if (IsTxFramePending == true)
	{
		if (AppSend() == true)
		{
			SYSLOG_I("App send data");
			IsTxFramePending = false;
		}
	}
}

void AppStop(void)
{

}

bool AppIsRun(void)
{
	return IsTxFramePending;
}


