//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
  
#include <string.h>

#include "Recovery.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "flash.h"
#include "FileFunc.h"


#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE   "RECOVERY:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

//******************************************************************************
// Private Types
//******************************************************************************

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
// Recovery check
//******************************************************************************
RECOVERY_RESULT RecoveryCheck(void)
{
  INFO_STRUCT Info;
  bool ResGetInfo;
  LT_FILE *f = LiteDiskFileOpen(RECOVERY_FILE_NAME);
  
  SYSLOG_I("CHECK");
  if (LiteDiskIsInit() == false)
  {
    SYSLOG_W("DISK NOT INIT");
    return RECOVERY_RESULT_MISSING;
  }
  if(!f)
  {
    SYSLOG_W("FILE NOT OPEN");
    return RECOVERY_RESULT_FAIL;    
  }
  if (FileCheckCrcFile(f, APP_SIZE) == false)
  {
    SYSLOG_E("BAD CRC");
    return RECOVERY_RESULT_FAIL;
  }  
  ResGetInfo = FileGetInfo(f, 0, APP_SIZE, &Info);
  SYSLOG_D("Get info res = %d, DevID=%d, Version: %d.%d.%d", Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
  if ((!ResGetInfo) || (Info.dev_id != DEV_ID)) // Ia iaoee eioi?iaoe? eee ia niaiaaaao Id
  {
    SYSLOG_E("BAD ID OR NOT VERSION");
    return RECOVERY_RESULT_FAIL;
  }
  SYSLOG_I("CHECK OK");
  return RECOVERY_RESULT_OK;
}

//******************************************************************************
// Recovery save
//******************************************************************************
RECOVERY_RESULT RecoverySave(void)
{
  int Result;
  uint32_t AmountWrited;
  uint8_t *pData;
  uint8_t Buff[256];
  LT_FILE *f = LiteDiskFileOpen(RECOVERY_FILE_NAME);

  SYSLOG_I("SAVE");
  if (LiteDiskIsInit() == false)
  {
    return RECOVERY_RESULT_MISSING;
  }
  if(!f)
  {
    SYSLOG_E("FILE NOT OPEN");
    return RECOVERY_RESULT_FAIL;    
  }  
  pData = (uint8_t*)(APP_START_ADDRESS);
  Result = LiteDiskFileClear(f); // Очищаем файл
  SYSLOG_I("CLEAR SIZE=%d", Result);
  if(Result < 0) return RECOVERY_RESULT_FAIL;
  for(AmountWrited = 0; AmountWrited < APP_SIZE;)//
  {
    memcpy(Buff, &pData[AmountWrited], sizeof(Buff));
    Result = LiteDiskFileWrite(f, AmountWrited, sizeof(Buff), Buff);
    if(Result == sizeof(Buff))
    {
      AmountWrited += Result;
    }
    else
    {
      SYSLOG_E("WRITE ERR=%d. AmountWrited = %d", Result, AmountWrited);
      return RECOVERY_RESULT_FAIL;
    }
  }
  SYSLOG_I("SAVE SIZE=%d", AmountWrited);
  return RECOVERY_RESULT_OK;
}        

//******************************************************************************
// Recovery app
//******************************************************************************
RECOVERY_RESULT RecoveryApp(void)
{
  LT_FILE *f = LiteDiskFileOpen(RECOVERY_FILE_NAME);
  
  if (LiteDiskIsInit() == false)
  {
    SYSLOG_W("DISK NOT INIT");
    return RECOVERY_RESULT_MISSING;
  }  
  if(!f)
  {
    SYSLOG_W("FILE NOT OPEN");
    return RECOVERY_RESULT_FAIL;    
  }    
  if (FlashProgramApp(APP_START_ADDRESS, APP_SIZE, f) != FLASH_OK)
  {
    SYSLOG_E("ERR WRITE");
    return RECOVERY_RESULT_FAIL;
  }
  return RECOVERY_RESULT_OK;
}

//******************************************************************************
// Recovery Delete
//******************************************************************************
RECOVERY_RESULT RecoveryDelete(void)
{
  LT_FILE *f = LiteDiskFileOpen(RECOVERY_FILE_NAME);
  int Result;
  
  SYSLOG_I("DELETE");
  if (LiteDiskIsInit() == false)
  {
    return RECOVERY_RESULT_MISSING;
  }
  if(!f)
  {
    SYSLOG_E("FILE NOT OPEN");
    return RECOVERY_RESULT_FAIL;    
  }  
  Result = LiteDiskFileClear(f); // Очищаем файл
  SYSLOG_I("CLEAR SIZE=%d", Result);  
  return RECOVERY_RESULT_OK;
}
