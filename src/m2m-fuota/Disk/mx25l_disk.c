//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "mx25l_disk.h"
#include "mx25l.h"
#include "delay.h"
#include "stddef.h"
#include "stdbool.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE   "MX25L:"
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

static int mx25l_disk_init(void *InitStr);
static int mx25l_disk_erase_sector(uint32_t Sector);
static int mx25l_disk_sector_read(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
static int mx25l_disk_sector_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);

//******************************************************************************
// Private Data
//******************************************************************************

//******************************************************************************
// Public Data
//******************************************************************************

const LT_DISK MX25L_DISK =
{
	.SectorSize = MX25L_DISK_SECTOR_SIZE,
	.TotalSectors = MX25L_DISK_SECTOR_TOTAL,
	.disk_initialize = mx25l_disk_init,
	.disk_sector_erase = mx25l_disk_erase_sector,
	.disk_sector_read = mx25l_disk_sector_read,
	.disk_sector_write = mx25l_disk_sector_write,
};

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Init flash
//******************************************************************************
static int mx25l_disk_init(void *InitStr)
{
  Spi_t *Spi;
  
  if (!InitStr) return DRESULT_PARERR;
  Spi = (Spi_t*)InitStr;
  mx25l_init(Spi);
  
  SYSLOG_I("Initialize");
  if(mx25l_readid(&mx25l_dev) !=0) 
  {
    SYSLOG_E("ERROR GET ID");
    return DRESULT_ERROR;	
  }
  return DRESULT_OK;
}

//******************************************************************************
// Clear flash sector
//******************************************************************************
static int mx25l_disk_erase_sector(uint32_t Sector)
{
  if (Sector >= MX25L_DISK_SECTOR_TOTAL) return DRESULT_PARERR;
  SYSLOG_D("Erase sector = %d", Sector);
  mx25l_sectorerase(&mx25l_dev, Sector);
  return DRESULT_OK;
}

//******************************************************************************
// Disk write data
//******************************************************************************
static int mx25l_disk_sector_write(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
  uint32_t Addr;
  uint32_t nBytes;
  uint32_t Amount;
  uint32_t nWrited = 0;
  
  if (Sector >= MX25L_DISK_SECTOR_TOTAL) return DRESULT_PARERR;
  if (Offs >= MX25L_DISK_SECTOR_SIZE) return DRESULT_PARERR;
  
  Addr = (Sector * MX25L_DISK_SECTOR_SIZE) + Offs;
  if (Size > (MX25L_DISK_SECTOR_SIZE - Offs)) Amount = (MX25L_DISK_SECTOR_SIZE - Offs);
  else Amount = Size;
  
  for (nWrited = 0; nWrited < Amount;)
  {
    nBytes = MIN((Amount - nWrited), (256 - ((Addr + nWrited) % 256)));
    SYSLOG_D("Addr = 0x%08x, nBytes = %d", (Addr + nWrited), nBytes);
    mx25l_pagewrite(&mx25l_dev,(uint8_t*)&Data[nWrited], (Addr + nWrited), nBytes);
    nWrited += nBytes;
  }
  
  return nWrited;
}

//******************************************************************************
// Disk read data
//******************************************************************************
static int mx25l_disk_sector_read(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
  uint32_t Addr;
  uint32_t nBytes;
  
  if (Sector >= MX25L_DISK_SECTOR_TOTAL) return DRESULT_PARERR;
  if (Offs >= MX25L_DISK_SECTOR_SIZE) return DRESULT_PARERR;

  Addr = (Sector * MX25L_DISK_SECTOR_SIZE) + Offs;
  if (Size > (MX25L_DISK_SECTOR_SIZE - Offs)) nBytes = (MX25L_DISK_SECTOR_SIZE - Offs);
  else nBytes = Size;  
  
  //SYSLOG_D("Read sector = %d, Addr = 0x%08x, nBytes = %d", Sector, Addr, nBytes);
  mx25l_byteread(&mx25l_dev,Data, Addr, nBytes);
  
  return nBytes;
}


//******************************************************************************
// Public Functions
//******************************************************************************


