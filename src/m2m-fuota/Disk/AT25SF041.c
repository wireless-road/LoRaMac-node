//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "at25sf041.h"
#include "delay.h"
#include "stddef.h"
#include "stdbool.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_NOT
#define LOG_MODULE   "AT25SF041:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define CMD_READ_SRB1				0x05
#define CMD_READ_SRB2				0x35
#define CMD_DEEP_POWER_DOWN	   		0xB9
#define CMD_RESUME_FROM_DPD	   		0xAB
#define CMD_READ_MID 		   		0x9F
#define CMD_BLOCK_ERASE_4K			0x20
#define CMD_WRITE_ENABLE			0x06
#define CMD_WRITE_DISABLE			0x04
#define CMD_READ_ARRAY 	 	   		0x03
#define CMD_WRITE_ARRAY 	 	   	0x02

#define TIMEOUT_STEP				5

//******************************************************************************
// Private Types
//******************************************************************************
typedef struct
{
	uint8_t opcode;
	uint8_t Addr[4];
	uint8_t SizeAddr;
	uint8_t SizeDummy;
} At25sf041_header;
//******************************************************************************
// Private Function Prototypes
//******************************************************************************
static int at25sf041_init(void *InitStr);
static int at25sf041_erase_sector(uint32_t Sector);
static int at25sf041_sector_read(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
static int at25sf041_sector_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
static int at25sf041_sector_re_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
//******************************************************************************
// Private Data
//******************************************************************************
static uint8_t Buff[AT25SF041_SECTOR_SIZE];
//******************************************************************************
// Public Data
//******************************************************************************

const LT_DISK AT25SF041_DISK =
{
	.SectorSize = AT25SF041_SECTOR_SIZE,
	.TotalSectors = AT25SF041_SECTOR_TOTAL,
	.DiskInitialize = at25sf041_init,
	.DiskSectorErase = at25sf041_erase_sector,
	.DiskSectorRead = at25sf041_sector_read,
	.DiskSectorWrite = at25sf041_sector_write,
	.DiskSectorReWrite = at25sf041_sector_re_write,
};

at25sf041_t at25sf041 = {0};

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Disk sharing
//******************************************************************************
static void AT25AT25SF041InOut(At25sf041_header *header, uint8_t *out, uint32_t sizOut, uint8_t *In, uint32_t sizIn)
{
    //NSS = 0;
    GpioWrite( &at25sf041.Nss, 0 );
    SpiInOut(at25sf041.Spi, header->opcode);
    for(int i = 0; i < header->SizeAddr; i++ )
    {
    	SpiInOut(at25sf041.Spi, header->Addr[i]);
    }
    for(int i = 0; i < header->SizeDummy; i++ )
    {
    	SpiInOut(at25sf041.Spi, 0x55);
    }
    for(int i = 0; out && i < sizOut; i++ )
    {
    	SpiInOut(at25sf041.Spi, out[i]);
    }
    for(int i = 0; In && i < sizIn; i++ )
    {
    	In[i] = SpiInOut(at25sf041.Spi, 0 );
    }
    //NSS = 1;
    GpioWrite( &at25sf041.Nss, 1 );
}

//******************************************************************************
// Waiting for disk readiness
//******************************************************************************
static bool AT25AT25SFWaitReady(uint32_t Timeout)
{
	uint32_t Wait = 0;
	At25sf041_header cmd;
	uint8_t Stat[2];
	cmd.opcode = CMD_READ_SRB1;
	cmd.SizeAddr = 0;
	cmd.SizeDummy = 0;
	do
	{
		AT25AT25SF041InOut(&cmd, NULL, 0, Stat, 2);
		DelayMs(TIMEOUT_STEP);
		Wait += TIMEOUT_STEP;
	}
	while((Stat[0] & 1) && (Wait < Timeout));
	//SYSLOG_D("STAT0 = 0x%X, STAT1=0x%X");
	if (Wait >= Timeout) return false;
	DelayMs(TIMEOUT_STEP);
	return true;
}

//******************************************************************************
// Reset write protection
//******************************************************************************
static bool AT25AT25SFResetWP(void)
{
	At25sf041_header cmd;

	cmd.opcode = CMD_WRITE_ENABLE;
	cmd.SizeAddr = 0;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, NULL, 0);
	return true;
}


//******************************************************************************
// Set write protection
//******************************************************************************
static bool AT25AT25SFSetWP(void)
{
	At25sf041_header cmd;

	cmd.opcode = CMD_WRITE_DISABLE;
	cmd.SizeAddr = 0;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, NULL, 1);
	return true;
}

//******************************************************************************
// Write page 256b
//******************************************************************************
static int at25sf041_page_write(uint32_t Addr, uint8_t *Data, uint16_t Size)
{
	At25sf041_header cmd;
	uint32_t Amount;
	uint16_t Offs;

	if (AT25AT25SFResetWP() != true) return DRESULT_WRPRT;
	Offs = (Addr % AT25SF041_PAGE_SIZE);
	if (Size > (AT25SF041_PAGE_SIZE - Offs)) Amount = (AT25SF041_PAGE_SIZE - Offs);
	else Amount = Size;
	SYSLOG_D("WRITE PAGE. Addr = 0x%08X, Size=%d", Addr, Amount);
	cmd.opcode = CMD_WRITE_ARRAY;
	cmd.Addr[0] = (uint8_t)(Addr >> 16);
	cmd.Addr[1] = (uint8_t)(Addr >> 8);
	cmd.Addr[2] = (uint8_t)(Addr >> 0);
	cmd.SizeAddr = 3;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, Data, Amount, NULL, 0);
	if (AT25AT25SFWaitReady(1000) != true)
	{
		return DRESULT_NOTRDY;
	}
	return Amount;
}

//******************************************************************************
// Init flash
//******************************************************************************
static int at25sf041_init(void *InitStr)
{
	At25sf041_header cmd;
	uint8_t Id[3] = {0};
 	uint8_t TestId[3] = {0x1F, 0x84, 0x01};

	if (!InitStr) return DRESULT_PARERR;
	GpioInit( &at25sf041.Nss, FLASH_DISK_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	GpioInit( &at25sf041.Power, FLASH_DISK_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
	at25sf041.Spi = (Spi_t*)InitStr;
	DelayMs(100);
	GpioInit( &at25sf041.Power, FLASH_DISK_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
	DelayMs(100);
	/*  Resume from Deep Power-Down */
	cmd.opcode = CMD_RESUME_FROM_DPD;
	cmd.SizeAddr = 0;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, NULL, 0);
	DelayMs(1000);
	/* Read Manufacturer and Device ID*/
	cmd.opcode = CMD_READ_MID;
	cmd.SizeAddr = 0;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, Id, sizeof(Id));
	SYSLOG_I("Manufacturer ID = %d, Device ID_1 = %d, Device ID_2 = %d", Id[0], Id[1], Id[2]);
	if ((Id[0] != TestId[0]) || (Id[1] != TestId[1]) || (Id[2] != TestId[2]))
	{
		return DRESULT_ERROR;
	}
	return DRESULT_OK;
}

//******************************************************************************
// Clear flash sector
//******************************************************************************
static int at25sf041_erase_sector(uint32_t Sector)
{
	At25sf041_header cmd;
	uint32_t addr;

	if (Sector >= AT25SF041_SECTOR_TOTAL) return DRESULT_PARERR;
	if (AT25AT25SFResetWP() != true) return DRESULT_WRPRT;
	if (AT25AT25SFWaitReady(1000) != true)
	{
		return DRESULT_NOTRDY;
	}
	addr = (Sector * AT25SF041_SECTOR_SIZE);
	cmd.opcode = CMD_BLOCK_ERASE_4K;
	cmd.Addr[0] = (uint8_t)(addr >> 16);
	cmd.Addr[1] = (uint8_t)(addr >> 8);
	cmd.Addr[2] = (uint8_t)(addr >> 0);
	cmd.SizeAddr = 3;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, NULL, 0);
	if (AT25AT25SFWaitReady(1000) != true)
	{
		AT25AT25SFSetWP();
		return DRESULT_NOTRDY;
	}
	AT25AT25SFSetWP();
	SYSLOG_D("CLEAD SECTOR. Addr = 0x%x", addr);
	return DRESULT_OK;
}

//******************************************************************************
// Disk write data
//******************************************************************************
static int at25sf041_sector_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
	  uint32_t Addr;
	  uint32_t nBytes;
	  uint32_t Amount;
	  uint32_t nWrited = 0;

	  if (Sector >= AT25SF041_SECTOR_TOTAL) return DRESULT_PARERR;
	  if (Offs >= AT25SF041_SECTOR_SIZE) return DRESULT_PARERR;

	  Addr = (Sector * AT25SF041_SECTOR_SIZE) + Offs;
	  if (Size > (AT25SF041_SECTOR_SIZE - Offs)) Amount = (AT25SF041_SECTOR_SIZE - Offs);
	  else Amount = Size;

	  for (nWrited = 0; nWrited < Amount;)
	  {
	    nBytes = MIN((Amount - nWrited), (256 - ((Addr + nWrited) % 256)));
	    SYSLOG_D("Addr = 0x%08x, nBytes = %d", (Addr + nWrited), nBytes);
	    at25sf041_page_write((Addr + nWrited), (uint8_t*)&Data[nWrited], nBytes);
	    nWrited += nBytes;
	  }

	  return nWrited;
}

//******************************************************************************
// Disk read data
//******************************************************************************
static int at25sf041_sector_read(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
	At25sf041_header cmd;
	uint32_t Addr;
	uint32_t Amount;

	if (Sector >= AT25SF041_SECTOR_TOTAL) return DRESULT_PARERR;
	if (Offs >= AT25SF041_SECTOR_SIZE) return DRESULT_PARERR;
	Addr = (Sector * AT25SF041_SECTOR_SIZE) + Offs;
	if (Size > (AT25SF041_SECTOR_SIZE - Offs)) Amount = (AT25SF041_SECTOR_SIZE - Offs);
	else Amount = Size;
	SYSLOG_D("READ DATA. Addr = 0x%08X.Size=%d", Addr, Amount);
	cmd.opcode = CMD_READ_ARRAY;
	cmd.Addr[0] = (uint8_t)(Addr >> 16);
	cmd.Addr[1] = (uint8_t)(Addr >> 8);
	cmd.Addr[2] = (uint8_t)(Addr >> 0);
	cmd.SizeAddr = 3;
	cmd.SizeDummy = 0;
	AT25AT25SF041InOut(&cmd, NULL, 0, Data, Amount);
	return Amount;
}

//******************************************************************************
// Disk rewrire data
//******************************************************************************
static int  at25sf041_sector_re_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
	int Result;
	uint32_t Amount;

	if (Sector >= AT25SF041_SECTOR_TOTAL) return DRESULT_PARERR;
	if (Offs >= AT25SF041_SECTOR_SIZE) return DRESULT_PARERR;
	if (Size > (AT25SF041_SECTOR_SIZE - Offs)) Amount = (AT25SF041_SECTOR_SIZE - Offs);
	else Amount = Size;

	Result = at25sf041_sector_read(Sector, 0 , AT25SF041_SECTOR_SIZE, Buff);
	if (Result != sizeof(Buff))
	{
		return DRESULT_ERROR;
	}
	Result = at25sf041_erase_sector(Sector);
	if (Result != DRESULT_OK) return Result;
	for (int i = 0; i < Amount; i++)
	{
		Buff[Offs + i] = Data[i];
	}
	Result = at25sf041_sector_write(Sector, 0 , AT25SF041_SECTOR_SIZE, Buff);
	if (Result != sizeof(Buff))
	{
		return DRESULT_ERROR;
	}

	return Amount;
}


//******************************************************************************
// Public Functions
//******************************************************************************

