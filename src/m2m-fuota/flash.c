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
#include "flash.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
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

FLASH_RESULT FlashProgramApp(uint32_t StartAddr, uint32_t Size, LT_FILE *f)
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
		Result = LiteDiskFileRead(f, Offs, sizeof(Buff), Buff);
		if (Result != sizeof(Buff))
		{
            SYSLOG_E("ERROR READ IMAGE. File:%s, Offs = %d", f->Name ,Offs);
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

FLASH_RESULT FlashSaveToFile(uint32_t StartAddr, uint32_t Size, LT_FILE *f)
{
	  int Result;
	  uint32_t AmountWrited;
	  uint8_t *pData;
	  uint8_t Buff[256];

	  SYSLOG_I("SAVE");
	  if (LiteDiskIsInit() == false)
	  {
	    return FLASH_ERRROR;
	  }
	  if(!f)
	  {
	    SYSLOG_E("FILE NOT OPEN");
	    return FLASH_ERRROR;
	  }
	  pData = (uint8_t*)(APP_START_ADDRESS);
	  Result = LiteDiskFileClear(f); // ������� ����
	  SYSLOG_I("CLEAR SIZE=%d", Result);
	  if(Result < 0) return FLASH_ERRROR;
	  for(AmountWrited = 0; AmountWrited < APP_SIZE;)//
	  {
	    memcpy(Buff, &pData[AmountWrited], sizeof(Buff));
	    Result = LiteDiskFileWrite(f, AmountWrited, sizeof(Buff), Buff);
	    if(Result == sizeof(Buff))
	    {
	      AmountWrited += Result;
	    }
	    else
	    {
	      SYSLOG_E("WRITE ERR=%d. AmountWrited = %d", Result, AmountWrited);
	      return FLASH_ERRROR;
	    }
	  }
	  SYSLOG_I("SAVE SIZE=%d", AmountWrited);
	  return FLASH_OK;
}
