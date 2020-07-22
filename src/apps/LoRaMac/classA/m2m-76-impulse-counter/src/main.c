/*!
 * \file      main.c
 *
 * \brief     FUOTA interop tests - test 01
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2018 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 */

/*! \file fuota-test-01/NucleoL073/main.c */

#include <stdio.h>
#include <stdlib.h>
#include "utilities.h"
#include "board.h"
#include "board-config.h"
#if defined( SX1261MBXBAS ) || defined( SX1262MBXCAS ) || defined( SX1262MBXDAS )
    #include "sx126x-board.h"
#elif defined( SX1272MB2DAS)
    #include "sx1272-board.h"
#elif defined( SX1276MB1LAS ) || defined( SX1276MB1MAS )
    #include "sx1276-board.h"
#endif
#include "gpio.h"
#include "uart.h"

#include "Commissioning.h"
#include "LmHandler.h"
#include "LmhpCompliance.h"
#include "LmhpClockSync.h"
#include "LmhpRemoteMcastSetup.h"
#include "FileLoader.h"
#include "version.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APP:"
#include "syslog.h"


#ifndef ACTIVE_REGION

#warning "No active region defined, LORAMAC_REGION_EU868 will be used as default."

#define ACTIVE_REGION LORAMAC_REGION_EU868

#endif

/*!
 * LoRaWAN default end-device class
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A

/*!
 * Defines the application data transmission duty cycle. 60s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            60000

/*!
 * Defines a random delay for application data transmission duty cycle. 5s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        5000

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE                           0//LORAMAC_HANDLER_ADR_OFF

/*!
 * Default datarate
 *
 * \remark Please note that LORAWAN_DEFAULT_DATARATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_DEFAULT_CONFIRMED_MSG_STATE         LORAMAC_HANDLER_UNCONFIRMED_MSG

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFFER_MAX_SIZE            242

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        false

/*!
 *
 */
typedef enum
{
    LORAMAC_HANDLER_TX_ON_TIMER,
    LORAMAC_HANDLER_TX_ON_EVENT,
}LmHandlerTxEvents_t;

/*!
 * User application data
 */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t TxTimer;

/*!
 * Timer to handle the state of LED1
 */
static TimerEvent_t Led1Timer;

/*!
 * Timer to handle the state of LED2
 */
static TimerEvent_t Led2Timer;

/*!
 * Timer to handle the state of LED beacon indicator
 */
static TimerEvent_t LedBeaconTimer;

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
#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
static void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection );
#else
static void OnSysTimeUpdate( void );
#endif
static void StartTxProcess( LmHandlerTxEvents_t txEvent );
static void UplinkProcess( void );


/*!
 * Function executed on TxTimer event
 */
static void OnTxTimerEvent( void* context );

/*!
 * Function executed on Led 1 Timeout event
 */
static void OnLed1TimerEvent( void* context );

/*!
 * Function executed on Led 2 Timeout event
 */
static void OnLed2TimerEvent( void* context );

/*!
 * \brief Function executed on Beacon timer Timeout event
 */
static void OnLedBeaconTimerEvent( void* context );

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
    .Region = ACTIVE_REGION,
    .AdrEnable = LORAWAN_ADR_STATE,
    .TxDatarate = LORAWAN_DEFAULT_DATARATE,
    .PublicNetworkEnable = LORAWAN_PUBLIC_NETWORK,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .DataBufferMaxSize = LORAWAN_APP_DATA_BUFFER_MAX_SIZE,
    .DataBuffer = AppDataBuffer
};

static LmhpComplianceParams_t LmhpComplianceParams =
{
    .AdrEnabled = LORAWAN_ADR_STATE,
    .DutyCycleEnabled = LORAWAN_DUTYCYCLE_ON,
    .StopPeripherals = NULL,
    .StartPeripherals = NULL,
};


/*!
 * Indicates if LoRaMacProcess call is pending.
 *
 * \warning If variable is equal to 0 then the MCU can be set in low power mode
 */
static volatile uint8_t IsMacProcessPending = 0;

static volatile uint8_t IsTxFramePending = 0;

/*
 * Indicates if the system time has been synchronized
 */
static volatile bool IsClockSynched = false;

/*
 * MC Session Started
 */
volatile bool IsMcSessionStarted = false;

/*!
 * LED GPIO pins objects
 */
extern Gpio_t Led1; // Tx
extern Uart_t Uart;


/*!
 * Main application entry point.
 */
int main( void )
{
    INFO_STRUCT Info;

    BoardInitMcu( );
    BoardInitPeriph( );
    
    TimerInit( &Led1Timer, OnLed1TimerEvent );
    TimerSetValue( &Led1Timer, 25 );

    TimerInit( &Led2Timer, OnLed2TimerEvent );
    TimerSetValue( &Led2Timer, 100 );

    TimerInit( &LedBeaconTimer, OnLedBeaconTimerEvent );
    TimerSetValue( &LedBeaconTimer, 5000 );

    SYSLOG_I("START");
    VersionRead(APP_START_ADDRESS, APP_SIZE, &Info);
    SYSLOG_I("DevId=%d, Version:%d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
    
    LiteDiskInit((LT_DISK*)&DISK, (void*)&SX1276.Spi, (LT_FILE_DEFS*)&FILE_TABLE);
    SYSLOG_I("Disk is init = %d", LiteDiskIsInit());  
    
    //TestSaveUpdate();

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
    LmHandlerPackageRegister( PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams );
    LmHandlerPackageRegister( PACKAGE_ID_CLOCK_SYNC, NULL );
    LmHandlerPackageRegister( PACKAGE_ID_REMOTE_MCAST_SETUP, NULL );
    if (FileLoaderStart() != true)
    {
      SYSLOG_E("ERROR START UPDATE TASK");
    }

    IsClockSynched = false;

    LmHandlerJoin( );

    StartTxProcess( LORAMAC_HANDLER_TX_ON_TIMER );


    while( 1 )
    {
        // Processes the LoRaMac events
        LmHandlerProcess( );

        // Process application uplinks management
        UplinkProcess( );
        
        // Update task
        FileLoaderProc();

        CRITICAL_SECTION_BEGIN( );
        if( IsMacProcessPending == 1 )
        {
            // Clear flag and prevent MCU to go into low power modes.
            IsMacProcessPending = 0;
        }
        else
        {
            // The MCU wakes up through events
            //BoardLowPowerHandler( );
        }
        CRITICAL_SECTION_END( );
    }
}

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
    if (params->CommissioningParams->IsOtaaActivation == true)
    {
      SYSLOG_I("JOINED. DevAddr = %08X", params->CommissioningParams->DevAddr);
    }
#if ( OVER_THE_AIR_ACTIVATION == 0 )    
    else
    {
      SYSLOG_I("JOINED. DevAddr = %08X", params->CommissioningParams->DevAddr);
    }
#endif
    
    if( params->Status == LORAMAC_HANDLER_ERROR )
    {
        LmHandlerJoin( );
    }
    else
    {
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
  SYSLOG_I("Switch to Class %c done.  Time = %d ", "ABC"[deviceClass],  curTime.Seconds);

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
            // Switch LED 2 ON
            GpioWrite( &Led1, 1 );
            break;
        }
    }
}

static void OnBeaconStatusChange( LoRaMAcHandlerBeaconParams_t* params )
{
    switch( params->State )
    {
        case LORAMAC_HANDLER_BEACON_RX:
        {
            TimerStart( &LedBeaconTimer );
            break;
        }
        case LORAMAC_HANDLER_BEACON_LOST:
        case LORAMAC_HANDLER_BEACON_NRX:
        {
            TimerStop( &LedBeaconTimer );
            break;
        }
        default:
        {
            break;
        }
    }
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

#if( LMH_SYS_TIME_UPDATE_NEW_API == 1 )
static void OnSysTimeUpdate( bool isSynchronized, int32_t timeCorrection )
{
    SysTime_t Time  = SysTimeGet();
    
    SYSLOG_I("SysTimeUpdate. New Time = %d. timeCorrection = %d", Time.Seconds, timeCorrection);
    IsClockSynched = true;
}
#else
static void OnSysTimeUpdate( void )
{
  SysTime_t Time  = SysTimeGet();
  
    SYSLOG_I("SysTimeUpdate. New Time = %d", Time.Seconds);
    IsClockSynched = true;
}
#endif

static void StartTxProcess( LmHandlerTxEvents_t txEvent )
{
    switch( txEvent )
    {
    default:
        // Intentional fall through
    case LORAMAC_HANDLER_TX_ON_TIMER:
        {
            // Schedule 1st packet transmission
            TimerInit( &TxTimer, OnTxTimerEvent );
            TimerSetValue( &TxTimer, APP_TX_DUTYCYCLE  + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND ) );
            OnTxTimerEvent( NULL );
        }
        break;
    case LORAMAC_HANDLER_TX_ON_EVENT:
        {
        }
        break;
    }
}

static void UplinkProcess( void )
{
	FILE_LOADER_INFO Info;
	FILE_LOADER_STAT Stat;
  
    LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

    if( LmHandlerIsBusy( ) == true )
    {
        return;
    }

    uint8_t isPending = 0;
    CRITICAL_SECTION_BEGIN( );
    isPending = IsTxFramePending;
    IsTxFramePending = 0;
    CRITICAL_SECTION_END( );
    if( isPending == 1 )
    {
        if( IsMcSessionStarted == false )
        {
        	Stat = FileLoaderGetStat(&Info);
            if(( Stat != FILE_LOADER_SUCCESS ) && ( Stat != FILE_LOADER_FAIL))
            {
                if( IsClockSynched == false )
                {
                    status = LmhpClockSyncAppTimeReq( );
                }
                else
                {
                    AppDataBuffer[0] = randr( 0, 255 );
                    AppDataBuffer[1] = randr( 0, 255 );
                    AppDataBuffer[2] = randr( 0, 255 );
                    AppDataBuffer[3] = randr( 0, 255 );
                    // Send random packet
                    LmHandlerAppData_t appData =
                    {
                        .Buffer = AppDataBuffer,
                        .BufferSize = 4,
                        .Port = 3,
                    };
                    status = LmHandlerSend( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
                }
            }
            else
            {
            	size_t pBuff = 0;
            	SYSLOG_I("Load info send. Stat=%d,Type=%d,CrcCalc=0x%08X,CrcGet=0x%08X,DataSize=%u", Stat, Info.Type, Info.CrcCalc, Info.CrcGet, Info.DataSize);
                AppDataBuffer[pBuff++] = 0x05; // FragDataBlockAuthReq
                AppDataBuffer[pBuff++] = (uint8_t)(Stat);
                AppDataBuffer[pBuff++] = (uint8_t)(Info.Type);
                AppDataBuffer[pBuff++] = Info.CrcCalc & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcCalc >> 8 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcCalc >> 16 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcCalc >> 24 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = Info.CrcGet & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcGet >> 8 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcGet >> 16 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.CrcGet >> 24 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = Info.DataSize & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.DataSize >> 8 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.DataSize >> 16 ) & 0x000000FF;
                AppDataBuffer[pBuff++] = ( Info.DataSize >> 24 ) & 0x000000FF;
                // Send FragAuthReq
                LmHandlerAppData_t appData =
                {
                    .Buffer = AppDataBuffer,
                    .BufferSize = pBuff,
                    .Port = 201
                };
                status = LmHandlerSend( &appData, LORAMAC_HANDLER_UNCONFIRMED_MSG );
            }
            if( status == LORAMAC_HANDLER_SUCCESS )
            {
                // Switch LED 1 ON
                GpioWrite( &Led1, 1 );
                TimerStart( &Led1Timer );
            }
        }
    }
}

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

/*!
 * Function executed on Led 1 Timeout event
 */
static void OnLed1TimerEvent( void* context )
{
    TimerStop( &Led1Timer );
    // Switch LED 1 OFF
    GpioWrite( &Led1, 0 );
}

/*!
 * Function executed on Led 2 Timeout event
 */
static void OnLed2TimerEvent( void* context )
{
    TimerStop( &Led2Timer );
    // Switch LED 2 ON
    GpioWrite( &Led1, 1 );
}

/*!
 * \brief Function executed on Beacon timer Timeout event
 */
static void OnLedBeaconTimerEvent( void* context )
{
    GpioWrite( &Led1, 1 );
    TimerStart( &Led2Timer );

    TimerStart( &LedBeaconTimer );
}
