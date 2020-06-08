//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include <stdio.h>
#include <stdbool.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "version.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "flash.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_INFO
#define LOG_MODULE   "FLASH:"
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

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Erasing the program memory area for downloadable firmware
//******************************************************************************
static bool FlashErase(uint32_t StartAddr, uint32_t Size)
{
	FLASH_EraseInitTypeDef EraseInit;
	uint32_t PageError;
	HAL_StatusTypeDef Result;

	EraseInit.PageAddress = StartAddr;
	EraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInit.NbPages = ((Size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE);
	Result = HAL_FLASHEx_Erase(&EraseInit, &PageError);
	if (Result != HAL_OK)
	{
		SYSLOG_E("ERASE ERROR. PageError=%d", PageError);
		return false;
	}
	SYSLOG_I("ERASE OK. PageError=%d", PageError);
	return true;
}

//******************************************************************************
// Write the program memory
//******************************************************************************
static bool FlashWrite(uint32_t StartAddr, uint32_t Size, uint8_t *Buff)
{
	uint32_t CheckData;
	uint32_t WriteData;

	for(int i = 0; i < Size; i += 4)
	{
		WriteData = *(uint32_t*)(Buff + i);
		/* Device voltage range supposed to be [2.7V to 3.6V], the operation will be done by byte */
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (StartAddr + i), WriteData) == HAL_OK)
		{
			CheckData = *(uint32_t*)(StartAddr + i);
			/* Check the written value */
		    if(CheckData != WriteData)
		    {
		      /* Flash content doesn't match SRAM content */
		      return false;
		    }
		}
		else
		{
			/* Error occurred while writing data in Flash memory */
			 return false;
		}
	}
	return true;
}

//******************************************************************************
// Public Functions
//******************************************************************************

FLASH_RESULT FlashProgramApp(uint32_t StartAddr, uint32_t Size, uint16_t FileID)
{
	uint8_t Buff[FLASH_PAGE_SIZE];
	uint32_t Offs;
	int Result;
	// Unlock flash
	HAL_FLASH_Unlock();
	// Erase flash
	if (FlashErase(StartAddr, Size) != true)
	{
		SYSLOG_E("ERROR ERASE FLASH");
		//HAL_FLASH_Lock();
		return FLASH_ERRROR;
	};
	// Write
	for (Offs = 0; Offs < Size; )
	{
		Result = LiteDiskFileRead(FileID, Offs, sizeof(Buff), Buff);
		if (Result != sizeof(Buff))
		{
			SYSLOG_E("ERROR READ IMAGE.FileID = %d,Offs = %d", FileID ,Offs);
			//HAL_FLASH_Lock();
			return FLASH_ERRROR;
		}
		if (FlashWrite((StartAddr + Offs), sizeof(Buff), Buff) != true)
		{
			SYSLOG_E("ERROR WRITE IMAGE.Addr = 0x%08X", (StartAddr + Offs));
			//HAL_FLASH_Lock();
			return FLASH_ERRROR;
		}
		SYSLOG_D("WRITE OK.Offs = %d", Offs);
		Offs += sizeof(Buff);
	}
	//HAL_FLASH_Lock();
	return FLASH_OK;
}

