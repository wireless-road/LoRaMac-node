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

#include "Scheduler.h"
#include "LoRaWan.h"
#include "Servises.h"
#include "FileLoader.h"
#include "TestApp.h"

#include "version.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "MAIN:"
#include "syslog.h"



/*!
 * Main application entry point.
 */
int main( void )
{
    INFO_STRUCT Info;

    BoardInitMcu( );
    BoardInitPeriph( );

    SYSLOG_I("START");
    VersionRead(APP_START_ADDRESS, APP_SIZE, &Info);
    SYSLOG_I("DevId=%d, Version:%d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
    
    LiteDiskInit((LT_DISK*)&DISK, (void*)&SX1276.Spi, (LT_FILE_DEFS*)&FILE_TABLE);
    SYSLOG_I("Disk is init = %d", LiteDiskIsInit());  
    
    ProcessInit("LoRaWan", &LoRaWanFunc, &LoRaWanTaskId);
    ProcessInit("Servises", &ServisesFunc, &ServisesTaskId);
    ProcessInit("FileLoader", &FileLoaderFunc, &FileLoaderTaskId);
    ProcessInit("App", &AppFunc, &AppTaskId);
    SchedulerStart();

    while( 1 )
    {
    	SchedulerProc();
        CRITICAL_SECTION_BEGIN( );
        /*if( IsMacProcessPending == 1 )
        {
            // Clear flag and prevent MCU to go into low power modes.
            IsMacProcessPending = 0;
        }
        else
        {
            // The MCU wakes up through events
            //BoardLowPowerHandler( );
        }*/
        CRITICAL_SECTION_END( );
    }
}



