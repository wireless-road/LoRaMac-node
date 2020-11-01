//******************************************************************************
//
//******************************************************************************

#ifndef __MX25L_DISK_H
#define __MX25L_DISK_H

//******************************************************************************
// Included Files
//******************************************************************************

#include "LiteDisk.h"
#include "board.h"
#include "board-config.h"
#include "spi.h"
#include "gpio.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define MX25L_DISK_SECTOR_SIZE	        4096
#define MX25L_DISK_SECTOR_TOTAL	        1024

//******************************************************************************
// Public Types
//******************************************************************************


#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

extern const LT_DISK MX25L_DISK;

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

#endif /* __MX25L_DISK_H */