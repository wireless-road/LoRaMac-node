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
#include "utilities.h"
#include "board.h"
#include "board-config.h"
#include "gpio.h"

#include "Commissioning.h"
#include "LmHandler.h"
#include "LmhpCompliance.h"
#include "LmhpClockSync.h"
#include "LmhpRemoteMcastSetup.h"
#include "LmhpFragmentation.h"
#include "version.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APP:"
#include "syslog.h"

/*!
 * MAC status strings
 */
const char* MacStatusStrings[] =
{
    "OK",                            // LORAMAC_STATUS_OK
    "Busy",                          // LORAMAC_STATUS_BUSY
    "Service unknown",               // LORAMAC_STATUS_SERVICE_UNKNOWN
    "Parameter invalid",             // LORAMAC_STATUS_PARAMETER_INVALID
    "Frequency invalid",             // LORAMAC_STATUS_FREQUENCY_INVALID
    "Datarate invalid",              // LORAMAC_STATUS_DATARATE_INVALID
    "Frequency or datarate invalid", // LORAMAC_STATUS_FREQ_AND_DR_INVALID
    "No network joined",             // LORAMAC_STATUS_NO_NETWORK_JOINED
    "Length error",                  // LORAMAC_STATUS_LENGTH_ERROR
    "Region not supported",          // LORAMAC_STATUS_REGION_NOT_SUPPORTED
    "Skipped APP data",              // LORAMAC_STATUS_SKIPPED_APP_DATA
    "Duty-cycle restricted",         // LORAMAC_STATUS_DUTYCYCLE_RESTRICTED
    "No channel found",              // LORAMAC_STATUS_NO_CHANNEL_FOUND
    "No free channel found",         // LORAMAC_STATUS_NO_FREE_CHANNEL_FOUND
    "Busy beacon reserved time",     // LORAMAC_STATUS_BUSY_BEACON_RESERVED_TIME
    "Busy ping-slot window time",    // LORAMAC_STATUS_BUSY_PING_SLOT_WINDOW_TIME
    "Busy uplink collision",         // LORAMAC_STATUS_BUSY_UPLINK_COLLISION
    "Crypto error",                  // LORAMAC_STATUS_CRYPTO_ERROR
    "FCnt handler error",            // LORAMAC_STATUS_FCNT_HANDLER_ERROR
    "MAC command error",             // LORAMAC_STATUS_MAC_COMMAD_ERROR
    "ClassB error",                  // LORAMAC_STATUS_CLASS_B_ERROR
    "Confirm queue error",           // LORAMAC_STATUS_CONFIRM_QUEUE_ERROR
    "Multicast group undefined",     // LORAMAC_STATUS_MC_GROUP_UNDEFINED
    "Unknown error",                 // LORAMAC_STATUS_ERROR
};

/*!
 * MAC event info status strings.
 */
const char* EventInfoStatusStrings[] =
{
    "OK",                            // LORAMAC_EVENT_INFO_STATUS_OK
    "Error",                         // LORAMAC_EVENT_INFO_STATUS_ERROR
    "Tx timeout",                    // LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT
    "Rx 1 timeout",                  // LORAMAC_EVENT_INFO_STATUS_RX1_TIMEOUT
    "Rx 2 timeout",                  // LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT
    "Rx1 error",                     // LORAMAC_EVENT_INFO_STATUS_RX1_ERROR
    "Rx2 error",                     // LORAMAC_EVENT_INFO_STATUS_RX2_ERROR
    "Join failed",                   // LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL
    "Downlink repeated",             // LORAMAC_EVENT_INFO_STATUS_DOWNLINK_REPEATED
    "Tx DR payload size error",      // LORAMAC_EVENT_INFO_STATUS_TX_DR_PAYLOAD_SIZE_ERROR
    "Downlink too many frames loss", // LORAMAC_EVENT_INFO_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS
    "Address fail",                  // LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL
    "MIC fail",                      // LORAMAC_EVENT_INFO_STATUS_MIC_FAIL
    "Multicast fail",                // LORAMAC_EVENT_INFO_STATUS_MULTICAST_FAIL
    "Beacon locked",                 // LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED
    "Beacon lost",                   // LORAMAC_EVENT_INFO_STATUS_BEACON_LOST
    "Beacon not found"               // LORAMAC_EVENT_INFO_STATUS_BEACON_NOT_FOUND
};

#ifndef ACTIVE_REGION

#warning "No active region defined, LORAMAC_REGION_EU868 will be used as default."

#define ACTIVE_REGION LORAMAC_REGION_EU868

#endif

/*!
 * LoRaWAN default end-device class
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A

/*!
 * Defines the application data transmission duty cycle. 30s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            40000

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
#define LORAWAN_ADR_STATE                           LORAMAC_HANDLER_ADR_ON

/*!
 * Default datarate
 *
 * \remark Please note that LORAWAN_DEFAULT_DATARATE is used only when ADR is disabled 
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_3

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
static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq );
static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq );
static void OnJoinRequest( LmHandlerJoinParams_t* params );
static void OnTxData( LmHandlerTxParams_t* params );
static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params );
static void OnClassChange( DeviceClass_t deviceClass );
static void OnBeaconStatusChange( LoRaMAcHandlerBeaconParams_t* params );
static void OnSysTimeUpdate( void );
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static uint8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size );
static uint8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size );
#endif
static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost );
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone( int32_t status, uint32_t size );
#else
static void OnFragDone( int32_t status, uint8_t *file, uint32_t size );
#endif
static void StartTxProcess( LmHandlerTxEvents_t txEvent );
static void UplinkProcess( void );

/*!
 * Computes a CCITT 32 bits CRC
 *
 * \param [IN] buffer   Data buffer used to compute the CRC
 * \param [IN] length   Data buffer length
 * 
 * \retval crc          The computed buffer of length CRC
 */
static uint32_t Crc32( uint8_t *buffer, uint16_t length );

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
    .GetUniqueId = BoardGetUniqueId,
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
 * Defines the maximum size for the buffer receiving the fragmentation result.
 *
 * \remark By default FragDecoder.h defines:
 *         \ref FRAG_MAX_NB   21
 *         \ref FRAG_MAX_SIZE 50
 *
 *         FileSize = FRAG_MAX_NB * FRAG_MAX_SIZE
 *
 *         If bigger file size is to be received or is fragmented differently
 *         one must update those parameters.
 */
#define UNFRAGMENTED_DATA_SIZE                     ( 21 * 50 )

/*
 * Un-fragmented data storage.
 */
static uint8_t UnfragmentedData[UNFRAGMENTED_DATA_SIZE];

static LmhpFragmentationParams_t FragmentationParams =
{
#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
    .DecoderCallbacks = 
    {
        .FragDecoderWrite = FragDecoderWrite,
        .FragDecoderRead = FragDecoderRead,
    },
#else
    .Buffer = UnfragmentedData,
    .BufferSize = UNFRAGMENTED_DATA_SIZE,
#endif
    .OnProgress = OnFragProgress,
    .OnDone = OnFragDone
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
static volatile bool IsMcSessionStarted = false;

/*
 * Indicates if the file transfer is done
 */
static volatile bool IsFileTransferDone = false;

/*
 *  Received file computed CRC32
 */
static volatile uint32_t FileRxCrc = 0;

/*!
 * LED GPIO pins objects
 */
extern Gpio_t Led1; // Tx
extern Gpio_t Led2; // Rx

static void app_putchar(unsigned char ch)
{
	USART2_sendChar(ch);
}

/*!
 * Main application entry point.
 */
int main( void )
{
	INFO_STRUCT Info;

    BoardInitMcu( );
    SYSLOG_INIT(app_putchar); // Инициализируем вывод логов
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

    LmHandlerInit( &LmHandlerCallbacks, &LmHandlerParams );

    // The LoRa-Alliance Compliance protocol package should always be
    // initialized and activated.
    LmHandlerPackageRegister( PACKAGE_ID_COMPLIANCE, &LmhpComplianceParams );
    LmHandlerPackageRegister( PACKAGE_ID_CLOCK_SYNC, NULL );
    LmHandlerPackageRegister( PACKAGE_ID_REMOTE_MCAST_SETUP, NULL );
    LmHandlerPackageRegister( PACKAGE_ID_FRAGMENTATION, &FragmentationParams );

    IsClockSynched = false;
    IsFileTransferDone = false;

    LmHandlerJoin( );

    StartTxProcess( LORAMAC_HANDLER_TX_ON_TIMER );

    while( 1 )
    {
        // Processes the LoRaMac events
        LmHandlerProcess( );

        // Process application uplinks management
        UplinkProcess( );

        CRITICAL_SECTION_BEGIN( );
        if( IsMacProcessPending == 1 )
        {
            // Clear flag and prevent MCU to go into low power modes.
            IsMacProcessPending = 0;
        }
        else
        {
            // The MCU wakes up through events
            BoardLowPowerHandler( );
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
	SYSLOG_I("NvmContextChange:", state);
    if( state == LORAMAC_HANDLER_NVM_STORE )
    {
    	SYSLOG_D( "###### ============ CTXS STORED ============ ######" );
    }
    else
    {
    	SYSLOG_D( "###### =========== CTXS RESTORED =========== ######" );
    }
    SYSLOG_I("======================================================\n");
}

static void OnNetworkParametersChange( CommissioningParams_t* params )
{
	SYSLOG_I("=========NetworkParametersChang==============");
    SYSDUMP_D( "DevEui      :", params->DevEui, 8 );
    SYSDUMP_D( "AppEui      :", params->JoinEui, 8 );
    // For 1.0.x devices the AppKey corresponds to NwkKey
    SYSDUMP_D( "AppKey      :", params->NwkKey, 16 );
    SYSLOG_I("======================================================\n");
}

static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq )
{
	SYSLOG_I( "###### =========== MCPS-Request ============ ######" );
    switch( mcpsReq->Type )
    {
        case MCPS_CONFIRMED:
        {
        	SYSLOG_D( "######            MCPS_CONFIRMED             ######");
            break;
        }
        case MCPS_UNCONFIRMED:
        {
        	SYSLOG_D( "######           MCPS_UNCONFIRMED            ######");
            break;
        }
        case MCPS_PROPRIETARY:
        {
        	SYSLOG_D( "######           MCPS_PROPRIETARY            ######");
            break;
        }
        default:
        {
        	SYSLOG_D( "######                MCPS_ERROR             ######");
            break;
        }
    }
    SYSLOG_D( "STATUS      : %s", MacStatusStrings[status] );
    SYSLOG_I("======================================================\n");
}

static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq )
{
	   SYSLOG_I( "###### =========== MLME-Request ============ ######" );
	   switch( mlmeReq->Type )
	    {
	        case MLME_JOIN:
	        {
	        	SYSLOG_D( "######               MLME_JOIN               ######");
	            break;
	        }
	        case MLME_LINK_CHECK:
	        {
	        	SYSLOG_D( "######            MLME_LINK_CHECK            ######");
	            break;
	        }
	        case MLME_DEVICE_TIME:
	        {
	        	SYSLOG_D( "######            MLME_DEVICE_TIME           ######");
	            break;
	        }
	        case MLME_TXCW:
	        {
	        	SYSLOG_D( "######               MLME_TXCW               ######");
	            break;
	        }
	        case MLME_TXCW_1:
	        {
	        	SYSLOG_D( "######               MLME_TXCW_1             ######");
	            break;
	        }
	        default:
	        {
	        	SYSLOG_D( "######              MLME_UNKNOWN             ######");
	            break;
	        }
	    }
	   SYSLOG_D( "STATUS      : %s", MacStatusStrings[status] );
	   SYSLOG_I("======================================================\n");
}

static void OnJoinRequest( LmHandlerJoinParams_t* params )
{
	SYSLOG_I("OnJoinRequest");
    if( params->CommissioningParams->IsOtaaActivation == true )
    {
        if( params->Status == LORAMAC_HANDLER_SUCCESS )
        {
        	SYSLOG_I( "###### ===========   JOINED     ============ ######" );
        	SYSLOG_D( "OTAA" );
        	SYSLOG_D( "DevAddr     :  %08X", params->CommissioningParams->DevAddr );
        	SYSLOG_D( "DATA RATE   : DR_%d", params->Datarate );
        	SYSLOG_I("======================================================\n");
        }
    }
#if ( OVER_THE_AIR_ACTIVATION == 0 )
    else
    {
    	SYSLOG_I( "###### ===========   JOINED     ============ ######\r\n" );
    	SYSLOG_D( "ABP" );
    	SYSLOG_D( "DevAddr     : %08X\r\n", params->CommissioningParams->DevAddr );
        SYSDUMP_D( "NwkSKey     :", params->CommissioningParams->FNwkSIntKey, 16 );
        SYSDUMP_D( "AppSKey     :", params->CommissioningParams->AppSKey, 16 );
        SYSLOG_I("======================================================\n");
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
	MibRequestConfirm_t mibGet;

	SYSLOG_I("========================ON TX DATA=========================");
    if( params->IsMcpsConfirm == 0 )
    {
    	SYSLOG_D( "MLME-Confirm STATUS : %s", EventInfoStatusStrings[params->Status] );
        return;
    }
    SYSLOG_D( "MCPS-Confirm  STATUS : %s", EventInfoStatusStrings[params->Status] );
    SYSLOG_D( "UPLINK FRAME  = %8u", params->UplinkCounter );
    SYSLOG_D( "CLASS : %c", "ABC"[LmHandlerGetCurrentClass( )] );
    SYSLOG_D( "TX PORT : %d", params->AppData.Port );
    if( params->AppData.BufferSize != 0 )
    {
        if( params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG )
        {
        	SYSLOG_D( "CONFIRMED - %s", ( params->AckReceived != 0 ) ? "ACK" : "NACK" );
        }
        else
        {
        	SYSLOG_D( "UNCONFIRMED" );
        }
        SYSDUMP_D("TX DATA     : ", params->AppData.Buffer, params->AppData.BufferSize);
    }
    SYSLOG_D( "DATA RATE   : DR_%d", params->Datarate );
    mibGet.Type  = MIB_CHANNELS;
    if( LoRaMacMibGetRequestConfirm( &mibGet ) == LORAMAC_STATUS_OK )
    {
    	SYSLOG_D( "U/L FREQ    : %u", mibGet.Param.ChannelList[params->Channel].Frequency );
    }

    SYSLOG_D( "TX POWER    : %d", params->TxPower );
    SYSLOG_I("======================================================\n");
}

static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params )
{
    const char *slotStrings[] = { "1", "2", "C", "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot" };

    SYSLOG_I("==========ON RX DATA:===================");
    if( params->IsMcpsIndication == 0 )
    {
    	SYSLOG_D( "MLME-Indication STATUS : %s", EventInfoStatusStrings[params->Status] );
        return;
    }
    SYSLOG_D( "MCPS-Indication STATUS : %s", EventInfoStatusStrings[params->Status]);
    SYSLOG_D( "DOWNLINK FRAME = %8u", params->DownlinkCounter );
    SYSLOG_D( "RX WINDOW : %s", slotStrings[params->RxSlot] );
    SYSLOG_D( "RX PORT : %d", appData->Port );
    if( appData->BufferSize != 0 )
    {
    	SYSDUMP_D( "RX DATA     :",  appData->Buffer, appData->BufferSize);
    }
    SYSLOG_D( "DATA RATE   : DR_%d", params->Datarate );
    SYSLOG_D( "RX RSSI     : %d", params->Rssi );
    SYSLOG_D( "RX SNR      : %d", params->Snr );
    SYSLOG_I("======================================================\n");
}

static void OnClassChange( DeviceClass_t deviceClass )
{
	SYSLOG_I("###### ===== Switch to Class %c done.  ===== ######", "ABC"[deviceClass] );

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
            GpioWrite( &Led2, 1 );
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
        	SYSLOG_I( "###### ========= BEACON ACQUIRING ========== ######" );
            break;
        }
        case LORAMAC_HANDLER_BEACON_LOST:
        {
        	SYSLOG_I( "###### ============ BEACON LOST ============ ######" );
            break;
        }
        case LORAMAC_HANDLER_BEACON_RX:
        {
        	SYSLOG_I( "###### ===== BEACON %8lu ==== ######", params->Info.Time.Seconds );
        	SYSLOG_D( "GW DESC     : %d", params->Info.GwSpecific.InfoDesc );
        	SYSDUMP_D( "GW INFO     : ",  params->Info.GwSpecific.Info, 6);
        	SYSLOG_D( "FREQ        : %u", params->Info.Frequency );
        	SYSLOG_D( "DATA RATE   : DR_%d", params->Info.Datarate );
        	SYSLOG_D( "RX RSSI     : %d", params->Info.Rssi );
        	SYSLOG_D( "RX SNR      : %d", params->Info.Snr );
            break;
        }
        case LORAMAC_HANDLER_BEACON_NRX:
        {
        	SYSLOG_I( "###### ======== BEACON NOT RECEIVED ======== ######" );
            break;
        }
    }
}

static void OnSysTimeUpdate( void )
{
	SYSLOG_I("SysTimeUpdate");
    IsClockSynched = true;
}

#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static uint8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size )
{
	SYSLOG_I("DecoderWrite. addr = %d, size = %d");
	SYSDUMP_D("data: ", data, size);
    if( size >= UNFRAGMENTED_DATA_SIZE )
    {
    	SYSLOG_E("ERROR");
        return -1; // Fail
    }
    for(uint32_t i = 0; i < size; i++ )
    {
        UnfragmentedData[addr + i] = data[i];
    }
    SYSLOG_I("DONE");
    return 0; // Success
}

static uint8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size )
{
	SYSLOG_I("DecoderRead. addr = %d, size = %d");

    if( size >= UNFRAGMENTED_DATA_SIZE )
    {
    	SYSLOG_E("ERROR");
        return -1; // Fail
    }
    for(uint32_t i = 0; i < size; i++ )
    {
        data[i] = UnfragmentedData[addr + i];
    }
    SYSDUMP_D("data: ", data, size);
    SYSLOG_I("DONE");
    return 0; // Success
}
#endif

static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost )
{
    // Switch LED 2 OFF for each received downlink
    GpioWrite( &Led2, 0 );
    TimerStart( &Led2Timer );

    SYSLOG_I( "\r\n###### =========== FRAG_DECODER ============ ######\r\n" );
    SYSLOG_I( "######               PROGRESS                ######\r\n");
    SYSLOG_I( "###### ===================================== ######\r\n");
    SYSLOG_I( "RECEIVED    : %5d / %5d Fragments\r\n", fragCounter, fragNb );
    SYSLOG_I( "              %5d / %5d Bytes\r\n", fragCounter * fragSize, fragNb * fragSize );
    SYSLOG_I( "LOST        :       %7d Fragments\r\n\r\n", fragNbLost );
}

#if( FRAG_DECODER_FILE_HANDLING_NEW_API == 1 )
static void OnFragDone( int32_t status, uint32_t size )
{
    FileRxCrc = Crc32( UnfragmentedData, size );
    IsFileTransferDone = true;
    // Switch LED 2 OFF
    GpioWrite( &Led2, 0 );

    SYSLOG_I( "\r\n###### =========== FRAG_DECODER ============ ######\r\n" );
    SYSLOG_I( "######               FINISHED                ######\r\n");
    SYSLOG_I( "###### ===================================== ######\r\n");
    SYSLOG_I( "STATUS      : %ld\r\n", status );
    SYSLOG_I( "CRC         : %08lX\r\n\r\n", FileRxCrc );
}
#else
static void OnFragDone( int32_t status, uint8_t *file, uint32_t size )
{
    FileRxCrc = Crc32( file, size );
    IsFileTransferDone = true;
    // Switch LED 2 OFF
    GpioWrite( &Led2, 0 );

    SYSLOG_I( "\r\n###### =========== FRAG_DECODER ============ ######\r\n" );
    SYSLOG_I( "######               FINISHED                ######\r\n");
    SYSLOG_I( "###### ===================================== ######\r\n");
    SYSLOG_I( "STATUS      : %ld\r\n", status );
    SYSLOG_I( "CRC         : %08lX\r\n\r\n", FileRxCrc );
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
            if( IsFileTransferDone == false )
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
                AppDataBuffer[0] = 0x05; // FragDataBlockAuthReq
                AppDataBuffer[1] = FileRxCrc & 0x000000FF;
                AppDataBuffer[2] = ( FileRxCrc >> 8 ) & 0x000000FF;
                AppDataBuffer[3] = ( FileRxCrc >> 16 ) & 0x000000FF;
                AppDataBuffer[4] = ( FileRxCrc >> 24 ) & 0x000000FF;

                // Send FragAuthReq
                LmHandlerAppData_t appData =
                {
                    .Buffer = AppDataBuffer,
                    .BufferSize = 5,
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
    GpioWrite( &Led2, 1 );
}

/*!
 * \brief Function executed on Beacon timer Timeout event
 */
static void OnLedBeaconTimerEvent( void* context )
{
    GpioWrite( &Led2, 1 );
    TimerStart( &Led2Timer );

    TimerStart( &LedBeaconTimer );
}

static uint32_t Crc32( uint8_t *buffer, uint16_t length )
{
    // The CRC calculation follows CCITT - 0x04C11DB7
    const uint32_t reversedPolynom = 0xEDB88320;

    // CRC initial value
    uint32_t crc = 0xFFFFFFFF;

    if( buffer == NULL )
    {
        return 0;
    }

    for( uint16_t i = 0; i < length; ++i )
    {
        crc ^= ( uint32_t )buffer[i];
        for( uint16_t i = 0; i < 8; i++ )
        {
            crc = ( crc >> 1 ) ^ ( reversedPolynom & ~( ( crc & 0x01 ) - 1 ) );
        }
    }

    return ~crc;
}
