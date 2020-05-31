//******************************************************************************
//
//******************************************************************************


//******************************************************************************
// Included Files
//******************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "version.h"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define APP_START_ADDRESS   0x08006000
#define APP_SIZE          	0x0001A000

#define BOOT_START_ADDRESS  0x08000000
#define BOOT_SIZE          	0x00006000

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
    "BOOT:CHECK_APP ",
    "BOOT:CHECK_UPDATE ",
    "BOOT:UPDATE ",
    "BOOT:START_APP ",
	"BOOT:RELOAD ",
	"BOOT:RECOVERY ",
	"BOOT:ERROR ",
};

static const char* BootStepResult[] =
{
	"ОК\n",
	"FAIL\n",
	"MISSING\n",
};

//******************************************************************************
// Private Data
//******************************************************************************

//******************************************************************************
// Public Data
//******************************************************************************

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
// Verify Destination Application
//******************************************************************************
static BOOT_RESULT BootloaderCheckApp(void)
{
	uint32_t CrcAppRead, CrcAppCalc;
	uint32_t *pCrc;
	VER_RESULT Res;
	INFO_STRUCT Info;
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
	return BOOT_OK;
}

//******************************************************************************
// Check for updates
//******************************************************************************
static BOOT_RESULT BootloaderCheckNeedUpdate(void)
{
	return BOOT_MISSING;
}

//******************************************************************************
// Updates app
//******************************************************************************
static BOOT_RESULT BootloaderAppUpdate(void)
{
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
// Load recovery
//******************************************************************************
static BOOT_RESULT BootloaderLoadRecovery(void)
{
	return BOOT_OK;
}

//******************************************************************************
// Load recovery
//******************************************************************************
static BOOT_RESULT BootloaderSaveRecovery(void)
{
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
	BOOT_RESULT Result;
	INFO_STRUCT Info;

	BootloaderInit(); // Инициализируем железо
	SYSLOG_INIT(boot_putchar); // Инициализируем вывод логов
	VersionRead(BOOT_START_ADDRESS, BOOT_SIZE, &Info);
	SYSLOG("BOOT:DevId=%d, BootVer:%d.%d.%d\n", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);

	while(Step != BOOT_STEP_ERROR)
	{
		SYSLOG(BootStepString[Step]);
		switch (Step)
		{
		case BOOT_STEP_CHECK_APP:
			Result = BootloaderCheckApp();
			if (Result == BOOT_OK) Step = BOOT_STEP_CHECK_UPDATE;
			else Step = BOOT_STEP_RECOVERY;
			break;
		case BOOT_STEP_CHECK_UPDATE:
			Result = BootloaderCheckNeedUpdate();
			if (Result == BOOT_OK) Step = BOOT_STEP_UPDATE;
			else Step = BOOT_STEP_START_APP;
			break;
		case BOOT_STEP_UPDATE:
			Result = BootloaderAppUpdate();
			if (Result == BOOT_OK) Step = BOOT_STEP_CHECK_APP;
			else if (Result == BOOT_MISSING) Step = BOOT_STEP_START_APP;
			else Step = BOOT_STEP_RECOVERY;
			break;
		case BOOT_STEP_START_APP:
			BootloaderStartApp();
			break;
		case BOOT_STEP_RELOAD:
			BoardResetMcu();
			break;
		case BOOT_STEP_RECOVERY:
			Step = BOOT_STEP_RELOAD;
			break;
		default:
			break;
		}
		SYSLOG(BootStepResult[Result]);
	}
	BoardResetMcu();
}
