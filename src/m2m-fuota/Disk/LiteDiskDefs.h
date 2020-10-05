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
#include "mx25l_disk.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************
#define MAX_FILES                       5

#define DISK                            AT25SF041_DISK

#define RECOVERY_FILE_NAME              "recovery"
#define RECOVERY_FILE_SIZE		(128*1024)

#define UPDATE_FILE_NAME                "update"
#define UPDATE_FILE_SIZE		(128*1024)

#define TMP_FILE_NAME                   "temp"
#define TMP_FILE_SIZE		        (128*1024)

#define LIST_FILE_NAME                   "list"
#define LIST_FILE_SIZE		        (24*1024)

#define CONF_FILE_NAME                   "config"
#define CONF_FILE_SIZE		        (4*1024)

//******************************************************************************
// Public Types
//******************************************************************************

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

extern const LT_FILE_DEFS FILE_TABLE[];

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
