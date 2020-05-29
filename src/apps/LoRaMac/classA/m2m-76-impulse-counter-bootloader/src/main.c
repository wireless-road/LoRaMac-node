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


//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#ifdef PRODUCTION
#define printf(fmt, ...) (0)
#endif

#define APP_START_ADDRESS       0x08006000
#define APP_SIZE          		0x0001A000

#define CRC32_POLYNOMIAL 0xedb88320ull

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
	"ОК\n\r",
	"FAIL\n\r",
	"MISSING\n\r",
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

	pCrc = (uint32_t*)(APP_START_ADDRESS + APP_SIZE - 4);
	CrcAppCalc = crc32_app(0, (uint8_t*)(APP_START_ADDRESS), (APP_SIZE - 4));
	CrcAppRead = *pCrc;

	if (CrcAppCalc != CrcAppRead)
	{
		return BOOT_FAIL;
	}

	/* Check app crc, id etc*/
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
static void BootloaderLoadRecovery(void)
{

}

//******************************************************************************
// Load recovery
//******************************************************************************
static void SaveRecovery(void)
{

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

	BootloaderInit(); // Инициализируем железо

	while(Step != BOOT_STEP_ERROR)
	{
		puts(BootStepString[Step]);
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
		puts(BootStepResult[Result]);
	}
	BoardResetMcu();
}
