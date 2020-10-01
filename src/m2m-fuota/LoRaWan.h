//******************************************************************************
//
//******************************************************************************

#ifndef __LORAWAN_H
#define __LORAWAN_H

//******************************************************************************
// Included Files
//******************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "LmHandler.h"
#include "LmhpRemoteMcastSetup.h"
#include "Scheduler.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define LORAWAN_DEFAULT_REGION LORAMAC_REGION_EU868

/*!
 * LoRaWAN default end-device class
 */
#define LORAWAN_DEFAULT_CLASS                       CLASS_A


/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_STATE                           0//LORAMAC_HANDLER_ADR_OFF

/*!
 * Default datarate
 *
 * \remark Please note that LORAWAN_DEFAULT_DATARATE is used only when ADR is disabled
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * User application data buffer size
 */
#define LORAWAN_DATA_BUFFER_SIZE            		242

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        false



//******************************************************************************
// Public Types
//******************************************************************************

typedef enum
{
    LORAWAN_TX_ON_TIMER,
	LORAWAN_TX_ON_EVENT,
}LoRaWanTxEvents_t;

#ifndef __ASSEMBLY__

//******************************************************************************
// Public Data
//******************************************************************************

extern uint32_t LoRaWanTaskId;
extern PROCESS_FUNC LoRaWanFunc;

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

void LoRaWanInit(void);

void LoRaWanTimeProc(void);

bool LoRaWanIsRun(void);

void LoRaWanStop(void);

bool LoRaWanSend( uint8_t Port, size_t Size, uint8_t *Data );
  
#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */

#endif /* __*_H */
