//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "LoRaWan.h"
#include "Servises.h"
#include "board.h"
#include "Commissioning.h"
#include "config.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "LORAWAN:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************


//******************************************************************************
// Private Types
//******************************************************************************

typedef struct
{
    LoRaMacRegion_t Region;
    bool AdrEnable;
    int8_t TxDatarate;
    bool PublicNetworkEnable;
    bool DutyCycleEnabled;
    bool Otaa;
} __attribute__((__packed__ )) sLoRaWanParamsConf;

//******************************************************************************
// Private Function Prototypes
//******************************************************************************
static void OnMacProcessNotify( void );
static void OnNvmContextChange( LmHandlerNvmContextStates_t state );
static void OnNetworkParametersChange( CommissioningParams_t* params );
static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn  );
static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn  );
static void OnJoinRequest( LmHandlerJoinParams_t* params );
static void OnTxData( LmHandlerTxParams_t* params );
static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params );
static void OnClassChange( DeviceClass_t deviceClass );
static void OnBeaconStatusChange( LoRaMAcHandlerBeaconParams_t* params );

//******************************************************************************
// Private Data
//******************************************************************************
/*!
 * LoRaWan data
 */
static uint8_t LoRaWanDataBuffer[LORAWAN_DATA_BUFFER_SIZE];

static LmHandlerCallbacks_t LmHandlerCallbacks =
{
    .GetBatteryLevel = BoardGetBatteryLevel,
    .GetTemperature = NULL,
    .GetRandomSeed = BoardGetRandomSeed,
    .OnMacProcess = OnMacProcessNotify,
    .OnNvmContextChange = OnNvmContextChange,
    .OnNetworkParametersChange = OnNetworkParametersChange,
    .OnMacMcpsRequest = OnMacMcpsRequest,
    .OnMacMlmeRequest = OnMacMlmeRequest,
    .OnJoinRequest = OnJoinRequest,
    .OnTxData = OnTxData,
    .OnRxData = OnRxData,
    .OnClassChange= OnClassChange,
    .OnBeaconStatusChange = OnBeaconStatusChange,
    .OnSysTimeUpdate = OnSysTimeUpdate
};

static LmHandlerParams_t LmHandlerParams =
{
    .DataBufferMaxSize = LORAWAN_DATA_BUFFER_SIZE,
    .DataBuffer = LoRaWanDataBuffer
};

static sLoRaWanParamsConf LoRaWanParamsConf =
{
	.Region = LORAWAN_DEFAULT_REGION,
	.AdrEnable = LORAWAN_ADR_STATE,
	.TxDatarate = LORAWAN_DEFAULT_DATARATE,
	.PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
	.DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
	.Otaa = OVER_THE_AIR_ACTIVATION,
};



/*!
 * Indicates if LoRaMacProcess call is pending.
 *
 * \warning If variable is equal to 0 then the MCU can be set in low power mode
 */
static volatile uint8_t IsMacProcessPending = 0;




/*
 * MC Session Started
 */
volatile bool IsMcSessionStarted = false;


/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent( void* context );

//******************************************************************************
// Public Data
//******************************************************************************

uint32_t LoRaWanTaskId;
PROCESS_FUNC LoRaWanFunc =
{
	.Init = LoRaWanInit,
	.Proc = LoRaWanTimeProc,
	.Stop = LoRaWanStop,
	.IsNeedRun = LoRaWanIsRun,
};

//******************************************************************************
// Private Functions
//******************************************************************************
static void OnMacProcessNotify( void )
{
    IsMacProcessPending = 1;
}

static void OnNvmContextChange( LmHandlerNvmContextStates_t state )
{
    SYSLOG_I("NvmContextChange:%d", state);
}

static void OnNetworkParametersChange( CommissioningParams_t* params )
{
    SYSLOG_I("NetworkParametersChang");
}

static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq, TimerTime_t nextTxIn )
{
    SYSLOG_I( "MCPS-Request. Status = %d", status);
}

static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq, TimerTime_t nextTxIn )
{
	   SYSLOG_I( "MLME-Request. Type = %d, Status = %d", mlmeReq->Type, status);
}

static void OnJoinRequest( LmHandlerJoinParams_t* params )
{
    SYSLOG_I("OnJoinRequest");

    if( params->Status == LORAMAC_HANDLER_ERROR )
    {
        LmHandlerJoin( );
    }
    else
    {
        if (params->CommissioningParams->IsOtaaActivation == true)
        {
          SYSLOG_I("OTAA JOINED. DevAddr = %08X", params->CommissioningParams->DevAddr);
        }
        else
        {
          SYSLOG_I("ABP. DevAddr = %08X", params->CommissioningParams->DevAddr);
        }
        LmHandlerRequestClass( LORAWAN_DEFAULT_CLASS );
    }
}

static void OnTxData( LmHandlerTxParams_t* params )
{
  SYSLOG_I("ON TX DATA. PORT = %d", params->AppData.Port);
}

static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params )
{
    const char *slotStrings[] = { "1", "2", "C", "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot" };

    SYSLOG_I("ON RX DATA. PORT = %d,  WINDOW : %s, SIZE = %d, COUNT = %d", appData->Port, slotStrings[params->RxSlot], appData->BufferSize, params->DownlinkCounter );
}

static void OnClassChange( DeviceClass_t deviceClass )
{
  SysTime_t curTime = SysTimeGet();
  SYSLOG_I("Switch to Class %c done.  Time = %d RxCount = %d", "ABC"[deviceClass],  curTime.Seconds);

    switch( deviceClass )
    {
        default:
        case CLASS_A:
        {
            IsMcSessionStarted = false;
            break;
        }
        case CLASS_B:
        {
            // Inform the server as soon as possible that the end-device has switched to ClassB
            LmHandlerAppData_t appData =
            {
                .Buffer = NULL,
                .BufferSize = 0,
                .Port = 0
            };
            LmHandlerSend( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
            IsMcSessionStarted = true;
            break;
        }
        case CLASS_C:
        {
            IsMcSessionStarted = true;
        }
    }
}

static void OnBeaconStatusChange( LoRaMAcHandlerBeaconParams_t* params )
{
    switch( params->State )
    {
        default:
        case LORAMAC_HANDLER_BEACON_ACQUIRING:
        {
            SYSLOG_I( "BEACON ACQUIRING" );
            break;
        }
        case LORAMAC_HANDLER_BEACON_LOST:
        {
            SYSLOG_I( "BEACON LOS" );
            break;
        }
        case LORAMAC_HANDLER_BEACON_RX:
        {
        	SYSLOG_I( "BEACON %d ==== ######", params->Info.Time.Seconds );
            break;
        }
        case LORAMAC_HANDLER_BEACON_NRX:
        {
            SYSLOG_I( "BEACON NOT RECEIVED" );
            break;
        }
    }
}


//******************************************************************************
// Public Functions
//******************************************************************************

void LoRaWanInit(void)
{
	uint32_t ConfIndex = 0;
	RESULT_CONF ConfResult;
	int ConfLen;
	// Загружаем настройки
	ConfResult = ConfigPartOpen(ID_CONF_LORA_PARAM, &ConfIndex); // Пытаемся открыть
	SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	if (ConfResult == CONF_NOT)
	{
		ConfResult = ConfigPartCreate(ID_CONF_LORA_PARAM, 256); // Если не получилось то создаем
		SYSLOG_I("CreateConf. ConfResult=%d", ConfResult, ConfIndex);
		ConfResult = ConfigPartOpen(ID_CONF_LORA_PARAM, &ConfIndex); // Пытаемся открыть
		SYSLOG_I("OpenConf. ConfResult=%d,ConfIndex=%d", ConfResult, ConfIndex);
	}
	if (ConfResult == CONF_OK)
	{
		ConfLen = ConfigPartRead(ConfIndex, sizeof(sLoRaWanParamsConf), &LoRaWanParamsConf);
		if (ConfLen != sizeof(sLoRaWanParamsConf))
		{
			ConfigPartWrite(ConfIndex, sizeof(sLoRaWanParamsConf), &LoRaWanParamsConf);
			ConfigPartRead(ConfIndex, sizeof(sLoRaWanParamsConf), &LoRaWanParamsConf);
		}
	}
	LmHandlerParams.Region = LoRaWanParamsConf.Region;
	LmHandlerParams.AdrEnable = LoRaWanParamsConf.AdrEnable;
	LmHandlerParams.TxDatarate = LoRaWanParamsConf.TxDatarate;
	LmHandlerParams.PublicNetworkEnable = LoRaWanParamsConf.PublicNetworkEnable;
	LmHandlerParams.DutyCycleEnabled = LoRaWanParamsConf.DutyCycleEnabled;
	LmHandlerParams.IsOtaaActivation = LoRaWanParamsConf.Otaa;


    if ( LmHandlerInit( &LmHandlerCallbacks, &LmHandlerParams ) != LORAMAC_HANDLER_SUCCESS )
    {
        SYSLOG_E( "LoRaMac wasn't properly initialized" );
        // Fatal error, endless loop.
        while ( 1 )
        {
        }
    }

    // Set system maximum tolerated rx error in milliseconds
    LmHandlerSetSystemMaxRxError( 20 );

    // The LoRa-Alliance Compliance protocol package should always be
    // initialized and activated.

    LmHandlerPackageRegister( PACKAGE_ID_REMOTE_MCAST_SETUP, NULL );

    SYSLOG_I("LoRaWan is init");

    LmHandlerJoin( );
}

void LoRaWanTimeProc(void)
{
    // Processes the LoRaMac events
    LmHandlerProcess( );
}

bool LoRaWanIsRun(void)
{
    return true;
}

void LoRaWanStop(void)
{

}

bool LoRaWanSend( uint8_t Port, size_t Size, uint8_t *Data )
{
    if( LmHandlerIsBusy( ) == true )
    {
        return false;
    }

    if( IsMcSessionStarted == true )
    {
        return false;
    }

    if (Size > LORAWAN_DATA_BUFFER_SIZE)
    {
        return false;
    }

    memcpy(LoRaWanDataBuffer, Data, Size);
    LmHandlerAppData_t appData =
    {
        .Buffer = LoRaWanDataBuffer,
        .BufferSize = Size,
        .Port = Port
    };
    if (LmHandlerSend( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG ) != LORAMAC_HANDLER_SUCCESS)
    {
    	return false;
    }
    return true;
}




