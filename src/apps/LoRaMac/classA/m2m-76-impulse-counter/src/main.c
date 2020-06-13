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
	SYSLOG_I("NvmContextChange = %d", state);
}

static void OnNetworkParametersChange( CommissioningParams_t* params )
{
	SYSLOG_I("NetworkParametersChang");
	SYSLOG_D("IsOtaaActivation = %d", params->IsOtaaActivation);
	SYSDUMP_D("DevEui", &params->DevEui[0], sizeof(params->DevEui));
	SYSDUMP_D("JoinEui", &params->JoinEui[0], sizeof(params->JoinEui));
#if( ABP_ACTIVATION_LRWAN_VERSION == ABP_ACTIVATION_LRWAN_VERSION_V10x )
	SYSDUMP_D("GenAppKey", &params->GenAppKey[0], sizeof(params->GenAppKey));
#else
	SYSDUMP_D("AppKey", &params->AppKey[0], sizeof(params->AppKey));
#endif
	SYSDUMP_D("NwkKey", &params->NwkKey[0], sizeof(params->NwkKey));
	SYSLOG_D("NetworkId = %d", params->NetworkId);
	SYSLOG_D("DevAddr = %d", params->DevAddr);
#if ( OVER_THE_AIR_ACTIVATION == 0 )
	SYSDUMP_D("FNwkSIntKey", &params->FNwkSIntKey[0], sizeof(params->FNwkSIntKey));
	SYSDUMP_D("SNwkSIntKey", &params->SNwkSIntKey[0], sizeof(params->SNwkSIntKey));
	SYSDUMP_D("NwkSEncKey", &params->NwkSEncKey[0], sizeof(params->NwkSEncKey));
	SYSDUMP_D("AppSKey", &params->AppSKey[0], sizeof(params->AppSKey));
#endif
}

static void OnMacMcpsRequest( LoRaMacStatus_t status, McpsReq_t *mcpsReq )
{
	SYSLOG_I("MacMcpsRequest status = %d, type = %d", status, mcpsReq->Type);
}

static void OnMacMlmeRequest( LoRaMacStatus_t status, MlmeReq_t *mlmeReq )
{
	SYSLOG_I("MacMlmeRequest status = %d, type = %d", status, mlmeReq->Type);
}

static void OnJoinRequest( LmHandlerJoinParams_t* params )
{
	SYSLOG_I("JoinRequest");
	SYSLOG_I("Datarate = %d, Status = %d");
	SYSLOG_D("CommissioningParams:");
	SYSLOG_D("IsOtaaActivation = %d", params->CommissioningParams->IsOtaaActivation);
	SYSDUMP_D("DevEui", &params->CommissioningParams->DevEui[0], sizeof(params->CommissioningParams->DevEui));
	SYSDUMP_D("JoinEui", &params->CommissioningParams->JoinEui[0], sizeof(params->CommissioningParams->JoinEui));
#if( ABP_ACTIVATION_LRWAN_VERSION == ABP_ACTIVATION_LRWAN_VERSION_V10x )
	SYSDUMP_D("GenAppKey", &params->CommissioningParams->GenAppKey[0], sizeof(params->CommissioningParams->GenAppKey));
#else
	SYSDUMP_D("AppKey", &params->CommissioningParams->AppKey[0], sizeof(params->CommissioningParams->AppKey));
#endif
	SYSDUMP_D("NwkKey", &params->CommissioningParams->NwkKey[0], sizeof(params->CommissioningParams->NwkKey));
	SYSLOG_D("NetworkId = %d", params->CommissioningParams->NetworkId);
	SYSLOG_D("DevAddr = %d", params->CommissioningParams->DevAddr);
#if ( OVER_THE_AIR_ACTIVATION == 0 )
	SYSDUMP_D("FNwkSIntKey", &params->CommissioningParams->FNwkSIntKey[0], sizeof(params->CommissioningParams->FNwkSIntKey));
	SYSDUMP_D("SNwkSIntKey", &params->CommissioningParams->SNwkSIntKey[0], sizeof(params->CommissioningParams->SNwkSIntKey));
	SYSDUMP_D("NwkSEncKey", &params->CommissioningParams->NwkSEncKey[0], sizeof(params->CommissioningParams->NwkSEncKey));
	SYSDUMP_D("AppSKey", &params->CommissioningParams->AppSKey[0], sizeof(params->CommissioningParams->AppSKey));
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
	SYSLOG_I("OnTxData");
	SYSLOG_D("Channel = %d, Datarate = %d, TxPower = %d, UplinkCounter= %d, MsgType = %d, Status = %d", params->Channel, params->Datarate, params->TxPower, params->UplinkCounter, params->MsgType, params->Status);
	SYSLOG_D("Port = %d, BufferSize = %d", params->AppData.Port, params->AppData.BufferSize);
	SYSDUMP_D("Buffer: ", params->AppData.Buffer, params->AppData.BufferSize);
}

static void OnRxData( LmHandlerAppData_t* appData, LmHandlerRxParams_t* params )
{
	SYSLOG_I("OnRxData");
	SYSLOG_D("RxSlot = %d, Datarate = %d, Rssi = %d, Snr = %d, DownlinkCounter = %d, Status = %d", params->RxSlot, params->Datarate, params->Rssi, params->Snr, params->DownlinkCounter, params->Status);
	SYSLOG_D("Port = %d, BufferSize = %d", appData->Port, appData->BufferSize);
	SYSDUMP_D("Buffer: ", appData->Buffer, appData->BufferSize);
}

static void OnClassChange( DeviceClass_t deviceClass )
{
	SYSLOG_I("ClassChange = %d", deviceClass);

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

    SYSLOG_I("BeaconStatusChang");
}

static void OnSysTimeUpdate( void )
{
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
                    AppDataBuffer[4] = randr( 0, 255 );
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
