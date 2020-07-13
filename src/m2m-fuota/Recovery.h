//******************************************************************************
//
//******************************************************************************

#ifndef __RECOVERY_H
#define __RECOVERY_H

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

typedef enum
{
  RECOVERY_RESULT_OK,
  RECOVERY_RESULT_FAIL,
  RECOVERY_RESULT_MISSING,
} RECOVERY_RESULT;

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
  
// Recovery check
RECOVERY_RESULT RecoveryCheck(void);

// Recovery save
RECOVERY_RESULT RecoverySave(void);

// Recovery app
RECOVERY_RESULT RecoveryApp(void);

RECOVERY_RESULT RecoveryDelete(void);
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __RECOVERY_H */