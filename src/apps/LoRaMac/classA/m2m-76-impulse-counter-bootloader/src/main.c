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
#include "version.h"
#include "syslog.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "flash.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define CRC32_POLYNOMIAL 	0xedb88320ull

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

typedef  void ( *pFunction )( void );

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

static const char* BootStepString[] =
{
    "BOOT:CHECK_APP",
    "BOOT:CHECK_UPDATE",
    "BOOT:UPDATE",
    "BOOT:START_APP",
	"BOOT:RELOAD",
	"BOOT:RECOVERY",
	"BOOT:ERROR",
};

static const char* BootStepResult[] =
{
	"OK\n",
	"FAIL\n",
	"MISSING\n",
};
static bool isDiskInit;

//******************************************************************************
// Private Data
//******************************************************************************

//******************************************************************************
// Public Data
//******************************************************************************

#ifdef BOOTLOADER
extern Spi_t Spi;
#endif

//******************************************************************************
// Private Functions
//******************************************************************************

static void boot_putchar(unsigned char ch)
{
	USART2_sendChar(ch);
}

static uint32_t crc32_app(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    uint8_t* current = (unsigned char *) buf;
    crc = ~crc;
    while (len--)
    {
        uint32_t j;
        crc ^= *current++;
        for (j = 0; j < 8; j++)
            if (crc & 1)
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            else
                crc = crc >> 1;
    }
    return ~crc;
}


//******************************************************************************
// Device initialization
//******************************************************************************
static void BootloaderInit(void)
{
    BoardInitMcu( );
    BoardInitPeriph( );
}

//******************************************************************************
// dISK initialization
//******************************************************************************
static bool BootloaderDiskInit(void)
{
	if (LiteDiskInit(&DISK, &Spi, &FILE_TABLE) != DRESULT_OK)
	{
		return false;
	}
	return true;
}


//******************************************************************************
// Verify Destination Application
//******************************************************************************
static BOOT_RESULT BootloaderCheckApp(INFO_STRUCT *InfoApp)
{
	uint32_t CrcAppRead, CrcAppCalc;
	uint32_t *pCrc;
	VER_RESULT Res;
	INFO_STRUCT Info;

	memset(InfoApp, 0, sizeof(INFO_STRUCT));
	/* Check app crc */
	pCrc = (uint32_t*)(APP_START_ADDRESS + APP_SIZE - 4);
	CrcAppCalc = crc32_app(0, (uint8_t*)(APP_START_ADDRESS), (APP_SIZE - 4));
	CrcAppRead = *pCrc;
	SYSLOG("CRC:CALC=0x%08X,READ=0x%08X\n", CrcAppCalc, CrcAppRead);
	if (CrcAppCalc != CrcAppRead)
	{
		return BOOT_FAIL;
	}
	/* Check app id */
	Res = VersionRead(APP_START_ADDRESS, APP_SIZE, &Info);
	SYSLOG("VER:Res=%d, DevId=%d, AppVer:%d.%d.%d\n", Res, Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
	if ((Res == VER_NOT) || (Info.dev_id != DEV_ID))
	{
		return BOOT_FAIL;
	}
	*InfoApp = Info;
	return BOOT_OK;
}

//******************************************************************************
// Start App
//******************************************************************************
static void BootloaderStartApp(void)
{
	pFunction JumpToApplication;
	uint32_t JumpAddress;

	JumpAddress = * ( volatile uint32_t* )( APP_START_ADDRESS + 4 );
	JumpToApplication = ( pFunction ) JumpAddress;
	/* Rebase the vector table base address  */
	SCB->VTOR = APP_START_ADDRESS;
	/* Initialize user application's Stack Pointer */
	__set_MSP( *( volatile uint32_t* ) APP_START_ADDRESS );
	/*Переход на основную программу*/
	JumpToApplication( );
}

//******************************************************************************
// Get info  recovery
//******************************************************************************
static bool BootloaderGetInfoFile(uint16_t FileId, INFO_STRUCT *Info)
{
	VER_STRUCT tmp;

	for(uint32_t position = 0; position < APP_SIZE; position += 4)
	{
		LiteDiskFileRead(FileId, position, sizeof(VER_STRUCT), (uint8_t*)&tmp);
		if((tmp.num == M_NUN) && (strncmp((char*)tmp.str, M_STR, 8) == 0))
		{
			*Info = tmp.info;
			return true;
		}
	}
	return false;
}

//******************************************************************************
// Check crc file
//******************************************************************************
static bool BootloaderCheckCrcFile(uint16_t FileId)
{
	uint32_t CrcCalc = 0;
	uint32_t CrcRead;
	uint8_t Buff[256];
	uint32_t AmountRead;
	int Result;
	for (AmountRead =0; AmountRead < APP_SIZE; ) //
	{
		Result = LiteDiskFileRead(RECOVERY_FILE_ID, AmountRead, sizeof(Buff), Buff);
		if(Result == sizeof(Buff))
		{
			AmountRead += Result;
			if (AmountRead < APP_SIZE)
			{
				CrcCalc = crc32_app(CrcCalc, Buff, Result);
			}
			else
			{
				CrcCalc = crc32_app(CrcCalc, Buff, (Result - 4));
				CrcRead = (uint32_t)(Buff[255] << 24) + (uint32_t)(Buff[254] << 16) + (uint32_t)(Buff[253] << 8) + (uint32_t)(Buff[252] << 0);
			}
		}
		else
		{
			SYSLOG("RESULT READ RECOVERY ERR=%d. AmountRead = %d\n", Result, AmountRead);
			return false;
		}
	}
	SYSLOG("RECOVERY CRC_CALC=0x%08X, CRC_READ=0x%08X\n", CrcCalc, CrcRead);
	return (CrcCalc == CrcRead);
}

//******************************************************************************
// Check recovery
//******************************************************************************
static BOOT_RESULT BootloaderCheckRecovery(void)
{
	INFO_STRUCT Info;
	bool ResGetInfo;

	if (isDiskInit == false)
	{
		return BOOT_MISSING;
	}
	SYSLOG("CHECK RECOVERY\n");
	if (BootloaderCheckCrcFile(RECOVERY_FILE_ID) == false)
	{
		return BOOT_FAIL;
	}
	ResGetInfo = BootloaderGetInfoFile(RECOVERY_FILE_ID, &Info);
	SYSLOG("Recovery get info res = %d, DevID=%d, Version: %d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
	if ((!ResGetInfo) || (Info.dev_id != DEV_ID))
	{
		return BOOT_FAIL;
	}
	return BOOT_OK;
}

//******************************************************************************
// Save recovery
//******************************************************************************
static BOOT_RESULT BootloaderSaveRecovery(void)
{
	int Result;
	uint32_t AmountWrited;
	uint8_t *pData;
	uint8_t Buff[256];

	if (isDiskInit == false)
	{
		return BOOT_MISSING;
	}
	SYSLOG("SAVE RECOVERY\n");
	pData = (uint8_t*)(APP_START_ADDRESS);
	Result = LiteDiskFileClear(RECOVERY_FILE_ID); // Очищаем файл
	SYSLOG("CLEAR RECOVERY SIZE=%d\n", Result);
	if(Result < 0) return BOOT_FAIL;
	for(AmountWrited = 0; AmountWrited < APP_SIZE;)//
	{
		memcpy(Buff, &pData[AmountWrited], sizeof(Buff));
		Result = LiteDiskFileWrite(RECOVERY_FILE_ID, AmountWrited, sizeof(Buff), Buff);
		if(Result == sizeof(Buff))
		{
			AmountWrited += Result;
		}
		else
		{
			SYSLOG("RESULT WRITE RECOVERY ERR=%d. AmountWrited = %d\n", Result, AmountWrited);
			return BOOT_FAIL;
		}
	}
	SYSLOG("RESULT WRITE RECOVERY COMPLEATE SIZE=%d\n", AmountWrited);
	return BOOT_OK;
}

//******************************************************************************
// Load recovery
//******************************************************************************
static BOOT_RESULT BootloaderLoadRecovery(void)
{
	if (FlashProgramApp(APP_START_ADDRESS, APP_SIZE, RECOVERY_FILE_ID) != FLASH_OK)
	{
		return BOOT_FAIL;
	}
	return BOOT_OK;
}

//******************************************************************************
// Check for updates
//******************************************************************************
static BOOT_RESULT BootloaderCheckUpdate(INFO_STRUCT *InfoApp)
{
	INFO_STRUCT Info;
	bool ResGetInfo;

	if (isDiskInit == false)
	{
		return BOOT_MISSING;
	}
	SYSLOG("CHECK RECOVERY\n");
	if (BootloaderCheckCrcFile(UPDATE_FILE_ID) == false)
	{
		SYSLOG("Update file bad crc\n");
		return BOOT_MISSING;
	}
	ResGetInfo = BootloaderGetInfoFile(UPDATE_FILE_ID, &Info);
	SYSLOG("Update get info res = %d, DevID=%d, Version: %d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
	if ((!ResGetInfo) || (Info.dev_id != DEV_ID)) // Не нашли информацию или не совпадает Id
	{
		return BOOT_MISSING;
	}
	if ((Info.version[0] == InfoApp->version[0]) && (Info.version[1] == InfoApp->version[1]) && (Info.version[2] == InfoApp->version[2]))
	{
		return BOOT_MISSING;
	}
	return BOOT_OK;
}

//******************************************************************************
// Updates app
//******************************************************************************
static BOOT_RESULT BootloaderAppUpdate(void)
{
	if (FlashProgramApp(APP_START_ADDRESS, APP_SIZE, UPDATE_FILE_ID) != FLASH_OK)
	{
		return BOOT_FAIL;
	}
	return BOOT_OK;
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
	SYSLOG_INIT(boot_putchar); // Инициализируем вывод логов
	VersionRead(BOOT_START_ADDRESS, BOOT_SIZE, &Info);
	SYSLOG("BOOT:DevId=%d, BootVer:%d.%d.%d\n", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
	isDiskInit = BootloaderDiskInit();
	SYSLOG("Result init disk = %d\n", isDiskInit);

	while(Step != BOOT_STEP_ERROR)
	{
		CurStep = Step;
		switch (Step)
		{
		case BOOT_STEP_CHECK_APP:
			Result = BootloaderCheckApp(&Info);
			if (Result == BOOT_OK)
			{
				if (BootloaderCheckRecovery() == BOOT_FAIL)
				{
					SYSLOG("Recovery file is damaged\n");
					SYSLOG("Write recovery result = %d\n", BootloaderSaveRecovery());
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
				SYSLOG("SAVE RECOVERY ERROR. UPDATE MISSING. START APP\n");
				Step = BOOT_STEP_START_APP;
			}
			else
			{
				Result = BootloaderAppUpdate();
				if (Result == BOOT_OK) Step = BOOT_STEP_RELOAD;
				else Step = BOOT_STEP_RECOVERY;
			}
			break;
		case BOOT_STEP_START_APP:
			SYSLOG("%s %s", BootStepString[Step], BootStepResult[BOOT_OK]);
			BootloaderStartApp();
			break;
		case BOOT_STEP_RELOAD:
			BoardResetMcu();
			break;
		case BOOT_STEP_RECOVERY:
			if (BootloaderCheckRecovery() == BOOT_FAIL)
			{
				SYSLOG("Recovery file is damaged\n");
				Step = BOOT_STEP_ERROR;
			}
			Result = BootloaderLoadRecovery();
			if (Result == BOOT_OK) Step = BOOT_STEP_RELOAD;
			else Step = BOOT_STEP_ERROR;
			break;
		default:
			break;
		}
		SYSLOG("BOOT:%s-%s", BootStepString[CurStep], BootStepResult[Result]);
	}
	// ERROS STATE
	while(1)
	{
		SYSLOG("FATAL ERROR!!!!\n");
	}
}
