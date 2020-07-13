//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include <string.h>
#include "stm32l0xx.h"
#include "utilities.h"
#include "board.h"
#include "delay.h"
#include "crc.h"
#include "AppLoader.h"
#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "APPLOADER:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Private Types
//******************************************************************************

typedef  void ( *pFunction )( void );

//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Verify Destination Application
//******************************************************************************
bool AppCheck(INFO_STRUCT *InfoApp, uint32_t StartAddr, uint32_t Size, uint8_t DevID)
{
  uint32_t CrcAppRead, CrcAppCalcIar, CrcAppCalcGcc;
  uint32_t *pCrc;
  VER_RESULT Res;
  INFO_STRUCT Info;

  SYSLOG_I("CHECK");
  memset(InfoApp, 0, sizeof(INFO_STRUCT));
  /* Check app crc */
  pCrc = (uint32_t*)(StartAddr + Size - 4);
  CrcAppCalcGcc = crc32_gcc(0, (uint8_t*)(StartAddr), (Size - 4));
  CrcAppCalcIar = crc32_iar(0, (uint8_t*)(StartAddr), (Size - 4));
  CrcAppRead = *pCrc;
  SYSLOG_D("CRC:CALC_GCC = 0x%08X, CALC_IAR = 0x%08X, READ = 0x%08X", CrcAppCalcGcc, CrcAppCalcIar, CrcAppRead);
  if ((CrcAppCalcGcc != CrcAppRead) && (CrcAppCalcIar != CrcAppRead))
  {
    SYSLOG_E("ERROR CRC");
    return false;
  }
  /* Check app id */
  Res = VersionRead(StartAddr, Size, &Info);
  if ((Res == VER_NOT) || (Info.dev_id != DevID))
  {
    SYSLOG_E("Error ID or not found version. Res = %d, Id = %d", Res, Info.dev_id);
    return false;	
  }
  *InfoApp = Info;
  SYSLOG_I("CHECK OK. DevId = %d, AppVer:%d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
  return true;
}

//******************************************************************************
// Start App
//******************************************************************************
void AppStart(uint32_t Addr, uint32_t Pause)
{
  pFunction JumpToApplication;
  uint32_t JumpAddress;

  SYSLOG_I("START APP...");
  DelayMs(Pause);
  JumpAddress = * ( volatile uint32_t* )( Addr + 4 );
  JumpToApplication = ( pFunction ) JumpAddress;
  /* Rebase the vector table base address  */
  SCB->VTOR = Addr;
  /* Initialize user application's Stack Pointer */
  __set_MSP( *( volatile uint32_t* ) Addr );
  /*Ia?aoia ia iniiaio? i?ia?aiio*/
  JumpToApplication( );
}

