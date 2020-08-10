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

typedef struct  _FILE_LOADER_INFO
{
	uint32_t Size;
    uint32_t SizeList;
} FILE_LOADER_INFO;

typedef struct _FILE_PART_DESCRIPTION
{
	uint32_t Addr;
	uint32_t Size;
}__attribute__((__packed__ )) FILE_PART_DESCRIPTION;

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
