//******************************************************************************
//
//******************************************************************************

#ifndef __FILE_LOADER_H
#define __FILE_LOADER_H

//******************************************************************************
// Included Files
//******************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "version.h"
//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Public Types
//******************************************************************************
typedef enum _FILE_LOADER_STAT
{
	FILE_LOADER_WAIT,
	FILE_LOADER_PROC,
	FILE_LOADER_ANALYSIS,
	FILE_LOADER_SUCCESS,
	FILE_LOADER_FAIL,
} FILE_LOADER_STAT;

typedef enum _FILE_LOADER_TYPE
{
	FILE_LOADER_UPDATE 	 = 1, // Файл обновления
	FILE_LOADER_PATCH	 = 2, // Исправление
	FILE_LOADER_SETTINGS = 3, // Настройки
} FILE_LOADER_TYPE;

typedef struct  _FILE_LOADER_INFO
{
	FILE_LOADER_TYPE Type;
	uint32_t CrcCalc;
	uint32_t CrcGet;
} FILE_LOADER_INFO;

typedef struct _FILE_LOADER_HEADER
{
  uint16_t 	DevID;
  uint16_t 	Type;
  uint32_t  FillingSize;
  uint32_t  FillingCrc;
} __attribute__((__packed__ )) FILE_LOADER_HEADER;

typedef struct _FILE_UPDATE_HEADER
{
	VER_STRUCT Ver; // Версия файла обновления
	uint32_t Size; // Размер данных
	uint32_t Crc; // CRC, который должен быть записан в конце флэши
} __attribute__((__packed__ )) FILE_UPDATE_HEADER;

typedef struct _FILE_PATCH_HEADER
{
	VER_STRUCT NewVer; // Версия на которую переходим
	VER_STRUCT VerOldFirm; // Версия которая должна быть у преведущей прошивки
	uint32_t NbParts; // Количество фрагментов, подлежаших обновлению
} __attribute__((__packed__ )) FILE_PATCH_HEADER;

typedef struct _PATCH_PART_DESCRIPTION
{
	uint32_t Offs;
	uint32_t Size;
}__attribute__((__packed__ )) _PATCH_PART_DESCRIPTION;

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
  
// Start update task
bool FileLoaderStart(void);

// Update task time proc
void FileLoaderProc(void);

// Stop update task
void FileLoaderStop(void);

FILE_LOADER_STAT FileLoaderGetStat(FILE_LOADER_INFO *Info);

  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __UPDATE_TASK_H*/
