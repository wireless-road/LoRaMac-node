//******************************************************************************
//
//******************************************************************************

#ifndef __AT25SF041_H
#define __AT25SF041_H


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

#define AT25SF041_SECTOR_SIZE	4096
#define AT25SF041_SECTOR_TOTAL	128
#define AT25SF041_PAGE_SIZE		256

//******************************************************************************
// Public Types
//******************************************************************************

typedef struct
{
    Gpio_t      Nss;
    Gpio_t      Power;
    Gpio_t		NssRadio;
    Spi_t       *Spi;
} at25sf041_t;

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************
extern at25sf041_t at25sf041;

extern const LT_DISK DISK;

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

int at25sf041_init(void *InitStr);
int at25sf041_erase_sector(uint16_t Sector);
int at25sf041_sector_read(uint8_t *Data, uint16_t Sector, uint16_t Offs, uint16_t Size);
int at25sf041_sector_write(uint8_t *Data, uint16_t Sector, uint16_t Offs, uint16_t Size);


#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */
