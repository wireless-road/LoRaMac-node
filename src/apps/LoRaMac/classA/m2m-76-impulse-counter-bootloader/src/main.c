//******************************************************************************
//
//******************************************************************************


//******************************************************************************
// Included Files
//******************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "stm32l0xx.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#ifdef PRODUCTION
#define printf(fmt, ...) (0)
#endif


//******************************************************************************
// Private Types
//******************************************************************************

typedef enum _BOOT_STEP
{
  BOOT_STEP_CHECK_APP, 		// �������� ����������
  BOOT_STEP_CHECK_UPDATE,	// �������� ������������� ����������
  BOOT_STEP_UPDATE,			// ����������
  BOOT_STEP_START_APP,		// ������ ����������
  BOOT_STEP_RELOAD,			// ������������
  BOOT_STEP_RECOVERY,		// �������������
  BOOT_STEP_ERROR,			// ������������ ������
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

//******************************************************************************
// Private Data
//******************************************************************************

//******************************************************************************
// Public Data
//******************************************************************************

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
// Verify Destination Application
//******************************************************************************
static BOOT_RESULT BootloaderCheckApp(void)
{
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

	JumpAddress = * ( volatile uint32_t* )( 0x8006000 + 4 );
	JumpToApplication = ( pFunction ) JumpAddress;
	/* ��������� ������ �������� ���������� */
	SCB->VTOR = 0x8006000;
	/* Initialize user application's Stack Pointer */
	__set_MSP( *( volatile uint32_t* ) 0x8006000 );
	/*������� �� �������� ���������*/
	Jump_To_Application( );
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

	BootloaderInit(); // �������������� ������

	while(Step != BOOT_STEP_ERROR)
	{
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
			break;
		case BOOT_STEP_START_APP:
			BootloaderStartApp();
			break;
		}

	}
}
