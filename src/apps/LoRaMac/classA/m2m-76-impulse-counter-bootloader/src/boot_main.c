//******************************************************************************
//
//******************************************************************************


//******************************************************************************
// Included Files
//******************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "spi.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "version.h"
#include "AppLoader.h"
#include "Update.h"
#include "Recovery.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "BOOT:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************


//******************************************************************************
// Private Types
//******************************************************************************

typedef enum _BOOT_STEP
{
  BOOT_STEP_CHECK_APP, 		// Проверка приложения
  BOOT_STEP_CHECK_UPDATE,	// Проверка необходимости обновления
  BOOT_STEP_UPDATE,			// Обновление
  BOOT_STEP_START_APP,		// Запуск приложения
  BOOT_STEP_RELOAD,			// Перезагрузка
  BOOT_STEP_RECOVERY,		// Востановление
  BOOT_STEP_ERROR,			// Неустранимая ошибка
} BOOT_STEP;

typedef enum _BOOT_RESULT
{
  BOOT_OK,
  BOOT_FAIL,
  BOOT_MISSING
} BOOT_RESULT;

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

static const char* BootStepString[] =
{
    "CHECK_APP",
    "CHECK_UPDATE",
    "UPDATE",
    "START_APP",
    "RELOAD",
    "RECOVERY",
    "ERROR",
};

static const char* BootStepResult[] =
{
  "OK",
  "FAIL",
  "MISSING",
};

//******************************************************************************
// Private Data
//******************************************************************************
 
//******************************************************************************
// Public Data
//******************************************************************************
extern Spi_t Spi;
//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Device initialization
//******************************************************************************
static void BootloaderInit(void)
{
    BoardInitMcu( );
    BoardInitPeriph( );
}

//******************************************************************************
// Check recovery
//******************************************************************************
static BOOT_RESULT BootloaderCheckRecovery()
{
  return  (BOOT_RESULT)RecoveryCheck();
}

//******************************************************************************
// Save recovery
//******************************************************************************
static BOOT_RESULT BootloaderSaveRecovery(void)
{
  return (BOOT_RESULT)RecoverySave();
}

//******************************************************************************
// Load recovery
//******************************************************************************
static BOOT_RESULT BootloaderLoadRecovery(void)
{
return (BOOT_RESULT)RecoveryApp();
}

//******************************************************************************
// Check for updates
//******************************************************************************
static BOOT_RESULT BootloaderCheckUpdate(INFO_STRUCT *InfoApp)
{
  
  return (BOOT_RESULT)UpdateCheck(InfoApp);
}

//******************************************************************************
// Updates app
//******************************************************************************
static BOOT_RESULT BootloaderAppUpdate(void)
{
  return (BOOT_RESULT)UpdateApp();
}

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Main application entry point.
//******************************************************************************
 int main( void )
{
	BOOT_STEP Step = BOOT_STEP_CHECK_APP;
	BOOT_STEP CurStep;
	BOOT_RESULT Result;
	INFO_STRUCT Info;

	BootloaderInit(); // Инициализируем железо
	VersionRead(BOOT_START_ADDRESS, BOOT_SIZE, &Info);
	SYSLOG_I("DevId = %d, BootVer:%d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
        LiteDiskInit((LT_DISK*)&DISK, (void*)&Spi, (LT_FILE_DEFS*)&FILE_TABLE);
	SYSLOG_I("Disk is init = %d", LiteDiskIsInit());
        
        //RecoveryDelete();

	while(Step != BOOT_STEP_ERROR)
	{
		CurStep = Step;
		switch (Step)
		{
		case BOOT_STEP_CHECK_APP:
			if (AppCheck(&Info, APP_START_ADDRESS, APP_SIZE, DEV_ID)) Result = BOOT_OK; else Result = BOOT_FAIL;
			if (Result == BOOT_OK)
			{
                          if (BootloaderCheckRecovery() != BOOT_OK)
                          {
                            BootloaderSaveRecovery();
                          }
                          Step = BOOT_STEP_CHECK_UPDATE;
			}
			else
			{
                          Step = BOOT_STEP_RECOVERY;
			}
			break;
		case BOOT_STEP_CHECK_UPDATE:
			Result = BootloaderCheckUpdate(&Info);
			if (Result == BOOT_OK) Step = BOOT_STEP_UPDATE;
			else Step = BOOT_STEP_START_APP;
			break;
		case BOOT_STEP_UPDATE:
			Result = BootloaderSaveRecovery();
			if (Result != BOOT_OK)
			{
				Step = BOOT_STEP_START_APP;
			}
			else
			{
				Result = BootloaderAppUpdate();
				if (Result == BOOT_OK) 
                                {
                                  UpdateDelete();
                                  Step = BOOT_STEP_RELOAD;
                                }
				else Step = BOOT_STEP_RECOVERY;
			}
			break;
		case BOOT_STEP_START_APP:
			AppStart(APP_START_ADDRESS, 5000);
			break;
		case BOOT_STEP_RELOAD:
			BoardResetMcu();
			break;
		case BOOT_STEP_RECOVERY:
			if (BootloaderCheckRecovery() == BOOT_OK)
			{
                          Result = BootloaderLoadRecovery();
                          if (Result == BOOT_OK) Step = BOOT_STEP_RELOAD;
                          else Step = BOOT_STEP_ERROR;                          
			}
                        else
                        {
                            Step = BOOT_STEP_ERROR;                          
                        }
			break;
		default:
			break;
		}
		SYSLOG_I("%s=%s", BootStepString[CurStep], BootStepResult[Result]);
	}
	// ERROS STATE
        SYSLOG_E("FATAL ERROR!!!!");
	while(1)
	{
		
	}
}
