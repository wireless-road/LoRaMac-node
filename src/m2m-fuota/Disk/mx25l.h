//******************************************************************************
//
//******************************************************************************

#ifndef _MX25l_H_
#define _MX25l_H_

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

//******************************************************************************
// Public Types
//******************************************************************************
typedef struct mx25l_dev_s
{
  uint8_t               sectorshift;
  uint8_t               pageshift;
  uint16_t              nsectors;
#if defined(CONFIG_MX25L_SECTOR512)
  uint8_t               flags;       /* Buffered sector flags */
  uint16_t              esectno;     /* Erase sector number in the cache */
  uint8_t          			*sector;      /* Allocated sector data */
#endif
} mx25l_dev_t;

extern mx25l_dev_t mx25l_dev;

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
extern void mx25l_init(Spi_t *Spi);
extern int mx25l_readid(mx25l_dev_t *priv);
extern void mx25l_waitwritecomplete(mx25l_dev_t *priv);
extern void mx25l_writeenable(mx25l_dev_t *priv);
extern void mx25l_writedisable(mx25l_dev_t *priv);
extern void mx25l_sectorerase(mx25l_dev_t *priv, uint32_t offset);
extern int  mx25l_chiperase(mx25l_dev_t *priv);
extern void mx25l_byteread(mx25l_dev_t *priv, uint8_t *buffer, uint32_t address, uint32_t nbytes);
extern void mx25l_pagewrite(mx25l_dev_t *priv, uint8_t *buffer, uint32_t address, uint32_t nbytes);

extern uint8_t mx25l_readstatus(mx25l_dev_t *priv);
extern uint8_t mx25l_readconfig(mx25l_dev_t *priv);
extern uint8_t mx25l_writestatconf(mx25l_dev_t *priv, uint8_t status, uint8_t config);
extern int mx25l_chipunlock(mx25l_dev_t *priv);

#if defined(CONFIG_MX25L_SECTOR512)
extern void mx25l_cacheflush(mx25l_dev_t *priv);
extern FAR uint8_t *mx25l_cacheread(mx25l_dev_t *priv, DWORD sector);
extern void mx25l_cacheerase(mx25l_dev_t *priv, DWORD sector);
extern void mx25l_cachewrite(mx25l_dev_t *priv, uint8_t *buffer, DWORD sector, UINT nsectors);
#endif  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* _HW_FLASH_H_ */
