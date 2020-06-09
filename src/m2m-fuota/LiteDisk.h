//******************************************************************************
//
//******************************************************************************

#ifndef __LITE_DISK_H
#define __LITE_DISK_H

//******************************************************************************
// Included Files
//******************************************************************************

#include <stdint.h>
#include <stdbool.h>

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define	NULL_FILE_ID			0

//******************************************************************************
// Public Types
//******************************************************************************

/* Results of Disk Functions */
typedef enum
{
	DRESULT_OK = 0,			/* 0: Successful */
	DRESULT_ERROR = -1,		/* 1: R/W Error */
	DRESULT_WRPRT = -2,		/* 2: Write Protected */
	DRESULT_NOTRDY = -3,	/* 3: Not Ready */
	DRESULT_PARERR = -4,	/* 4: Invalid Parameter */
} DRESULT;

typedef struct _LT_DISK
{
	uint16_t SectorSize;
	uint16_t TotalSectors;
	int (*disk_initialize)(void *InitStr);
	int (*disk_sector_read)(uint8_t *Data, uint16_t Sector, uint16_t Offs, uint16_t Size);
	int (*disk_sector_write)(uint8_t *Data, uint16_t Sector, uint16_t Offs, uint16_t Size);
	int (*disk_sector_erase)(uint16_t Sector);
} LT_DISK;

typedef struct _LT_FILE
{
	uint16_t Id;
	uint16_t StartSector;
	uint16_t TotalSectors;
} LT_FILE;



#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

//******************************************************************************
// Inline Functions
//******************************************************************************

//******************************************************************************
// Public Function Prototypes
//******************************************************************************

// Инициализация диска и упрощенной структуры хранения
DRESULT LiteDiskInit(LT_DISK *Disk, void *DiskInitStr, LT_FILE *Table);

// Проверка, что диск проинициализтрован
bool LiteDiskIsInit(void);

int LiteDiskFileClear(uint16_t FileID);
int LiteDiskFileWrite(uint16_t FileID, uint32_t Offs, uint32_t Size, uint8_t *Data);
int LiteDiskFileRead(uint16_t FileID, uint32_t Offs, uint32_t Size, uint8_t *Data);
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */
