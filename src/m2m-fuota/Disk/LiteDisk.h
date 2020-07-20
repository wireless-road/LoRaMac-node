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

//******************************************************************************
// Public Types
//******************************************************************************

/* Results of Disk Functions */
typedef enum
{
	DRESULT_OK = 0,		/* 0: Successful */
	DRESULT_ERROR = -1,	/* 1: R/W Error */
	DRESULT_WRPRT = -2,	/* 2: Write Protected */
	DRESULT_NOTRDY = -3,	/* 3: Not Ready */
	DRESULT_PARERR = -4,	/* 4: Invalid Parameter */
} DRESULT;

typedef struct _LT_DISK
{
	uint32_t SectorSize;
	uint32_t TotalSectors;
	int (*DiskInitialize)(void *InitStr);
	int (*DiskSectorRead)(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
	int (*DiskSectorWrite)(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
	int (*DiskSectorReWrite)(uint32_t Sector, uint32_t Offs, uint32_t Size, uint8_t *Data);
	int (*DiskSectorErase)(uint32_t Sector);
} LT_DISK;

typedef struct _LT_FILE
{
	char *Name;
        uint32_t Size;
	uint32_t StartSector;
	uint32_t AmountSectors;
} LT_FILE;

typedef struct _LT_FILE_DEFS
{
	char *Name;
        uint32_t Size;
} LT_FILE_DEFS;

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
DRESULT LiteDiskInit(LT_DISK *Disk, void *DiskInitStr, LT_FILE_DEFS *Table);

// Проверка, что диск проинициализтрован
bool LiteDiskIsInit(void);

LT_FILE *LiteDiskFileOpen(char *Name);
int LiteDiskFileClear(LT_FILE *f);
int LiteDiskFileWrite(LT_FILE *f, uint32_t Offs, uint32_t Size, uint8_t *Data);
int LiteDiskFileRead(LT_FILE *f, uint32_t Offs, uint32_t Size, uint8_t *Data);
int LiteDiskFileReWrite(LT_FILE *f, uint32_t Offs, uint32_t Size, uint8_t *Data);
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */
