//******************************************************************************
//
//******************************************************************************

#ifndef __LITE_DISK_DEFS_H
#define __LITE_DISK_DEFS_H

//******************************************************************************
// Included Files
//******************************************************************************

#include "LiteDisk.h"
#include "at25sf041.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define SECTOR_SIZE				AT25SF041_SECTOR_SIZE

#define UPDATE_FILE_ID			(NULL_FILE_ID + 1)
#define UPDATE_FILE_START		(0)
#define UPDATE_FILE_SIZE		(((104*1024)/SECTOR_SIZE) + 1)

#define RECOVERY0_FILE_ID		(UPDATE_FILE_ID + 1)
#define RECOVERY0_FILE_START	(UPDATE_FILE_START + UPDATE_FILE_SIZE)
#define RECOVERY0_FILE_SIZE		(((104*1024)/SECTOR_SIZE))

#define RECOVERY1_FILE_ID		(RECOVERY0_FILE_ID + 1)
#define RECOVERY1_FILE_START	(RECOVERY0_FILE_START + RECOVERY0_FILE_SIZE)
#define RECOVERY1_FILE_SIZE		(((104*1024)/SECTOR_SIZE))
//******************************************************************************
// Public Types
//******************************************************************************

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

extern const LT_FILE FILE_TABLE[];

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
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */
