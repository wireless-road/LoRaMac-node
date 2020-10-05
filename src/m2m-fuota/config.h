//******************************************************************************
//
//******************************************************************************

#ifndef __CONFIG_H
#define __CONFIG_H

//******************************************************************************
// Included Files
//******************************************************************************
#include <stdint.h>
#include <stddef.h>
//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Public Types
//******************************************************************************

typedef enum eRESULT_CONF
{
	CONF_OK = 0,
	CONF_ERRROR,
	CONF_NOT,
} RESULT_CONF;


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

RESULT_CONF ConfigFileOpen(void);
RESULT_CONF ConfigFileCreate(void);
RESULT_CONF ConfigPartOpen(char *Name, uint32_t *Indx);
RESULT_CONF ConfigPartCreate(char *Name, size_t Size, uint32_t *Indx);
int ConfigPartRead(uint32_t Indx, size_t size, void *Conf);
int ConfigPartWrite(uint32_t Indx, size_t size, void *Conf);
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __CONFIG_H */
