//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include "TestApp.h"

#include "LoRaWan.h"
#include "board.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APP:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

/*!
 * Defines the application data transmission duty cycle. 60s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            60000

/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        5000

//******************************************************************************
// Private Types
//******************************************************************************

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
    TimerSetValue( &TxTimer, APP_TX_DUTYCYCLE + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
    TimerStart( &TxTimer );
}


static bool AppSend(void)
{
	uint8_t Data[4];

	Data[0] = randr( 0, 255 );
	Data[1] = randr( 0, 255 );
	Data[2] = randr( 0, 255 );
	Data[3] = randr( 0, 255 );
	return LoRaWanSend(3, 4, Data);
}

//******************************************************************************
// Public Functions
//******************************************************************************

void AppInit(void)
{
    // Schedule 1st packet transmission
    TimerInit( &TxTimer, OnTxTimerEvent );
    TimerSetValue( &TxTimer, APP_TX_DUTYCYCLE  + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
    OnTxTimerEvent( NULL );
}

void AppTimeProc(void)
{
	if (IsTxFramePending == true)
	{
		if (AppSend() == true)
		{
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


