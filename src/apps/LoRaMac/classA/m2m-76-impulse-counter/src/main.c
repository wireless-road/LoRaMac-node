//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
#include <stdio.h>
#include "utilities.h"
#include "board.h"
#include "board-config.h"

#include "Commissioning.h"
#include "LmHandler.h"
#include "LmhpCompliance.h"
#include "LmhpClockSync.h"
#include "LmhpRemoteMcastSetup.h"
#include "LmhpFragmentation.h"
#include "LmHandlerMsgDisplay.h"

#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "flash.h"

#include "version.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APP:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Private Types
//******************************************************************************

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************

static bool isDiskInit = false;
//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Disk initialization
//******************************************************************************
static bool AppDiskInit(void)
{
	if (LiteDiskInit(&DISK, &SX1276.Spi, &FILE_TABLE) != DRESULT_OK)
	{
		return false;
	}
	return true;
}

//******************************************************************************
// Log putchar
//******************************************************************************
static void log_putchar(unsigned char ch)
{
	USART2_sendChar(ch);
}

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Main application entry point.
 //******************************************************************************
int main( void )
{
	INFO_STRUCT Info;

    BoardInitMcu( );
    BoardInitPeriph( );
    SYSLOG_INIT(log_putchar); // �������������� ����� �����
    SYSLOG_I("START");
	VersionRead(APP_START_ADDRESS, APP_SIZE, &Info);
	SYSLOG_I("DevId=%d, Version:%d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
	isDiskInit = AppDiskInit();
	SYSLOG_I("Result init disk = %d", isDiskInit);

	while(1)
	{

	}
}
