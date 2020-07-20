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
	FILE_LOADER_UPDATE 	 = 1, // ���� ����������
	FILE_LOADER_PATCH	 = 2, // �����������
	FILE_LOADER_SETTINGS = 3, // ���������
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
	VER_STRUCT Ver; // ������ ����� ����������
	uint32_t Size; // ������ ������
	uint32_t Crc; // CRC, ������� ������ ���� ������� � ����� �����
} __attribute__((__packed__ )) FILE_UPDATE_HEADER;

typedef struct _FILE_PATCH_HEADER
{
	VER_STRUCT NewVer; // ������ �� ������� ���������
	VER_STRUCT VerOldFirm; // ������ ������� ������ ���� � ���������� ��������
	uint32_t NbParts; // ���������� ����������, ���������� ����������
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
