//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include "Update.h"

#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "flash.h"
#include "FileFunc.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE   "UPDATE:"
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
// Updates app
//******************************************************************************
UPDATE_RESULT UpdateApp(void)
{
  LT_FILE *f = LiteDiskFileOpen(UPDATE_FILE_NAME);
  
  if (LiteDiskIsInit() == false)
  {
    SYSLOG_W("DISK NOT INIT");
    return UPDATE_RESULT_MISSING;
  }  
  if(!f)
  {
    SYSLOG_W("FILE NOT OPEN");
    return UPDATE_RESULT_FAIL;    
  }      
  if (FlashProgramApp(APP_START_ADDRESS, APP_SIZE, f) != FLASH_OK)
  {
    SYSLOG_E("ERR WRITE");
    return UPDATE_RESULT_FAIL;
  }
  return UPDATE_RESULT_OK;
}

//******************************************************************************
// Updates check
//******************************************************************************
UPDATE_RESULT UpdateCheck(INFO_STRUCT *InfoUpdate)
{
  INFO_STRUCT Info;
  bool ResGetInfo;
  LT_FILE *f = LiteDiskFileOpen(UPDATE_FILE_NAME);

  SYSLOG_I("CHECK");
  if (LiteDiskIsInit() == false)
  {
    SYSLOG_W("DISK NOT INIT");
    return UPDATE_RESULT_MISSING;
  }
  if(!f)
  {
    SYSLOG_W("FILE NOT OPEN");
    return UPDATE_RESULT_FAIL;    
  }    
  if (FileCheckCrcFile(f, APP_SIZE) == false)
  {
    SYSLOG_E("BAD CRC");
    return UPDATE_RESULT_FAIL;
  }
  ResGetInfo = FileGetInfo(f, 0, APP_SIZE, &Info);
  SYSLOG_D("Get info res = %d, DevID=%d, Version: %d.%d.%d", ResGetInfo, Info.dev_id, Info.version[0], Info.version[1], Info.version[2]);
  if ((!ResGetInfo) || (Info.dev_id != DEV_ID)) // Ia iaoee eioi?iaoe? eee ia niaiaaaao Id
  {
    SYSLOG_E("BAD ID OR NOT VERSION");
    return UPDATE_RESULT_FAIL;
  }

  *InfoUpdate = Info;
  SYSLOG_I("CHECK OK");
  return UPDATE_RESULT_OK;
}

//******************************************************************************
// Updates delete
//******************************************************************************
UPDATE_RESULT UpdateDelete(void)
{
  LT_FILE *f = LiteDiskFileOpen(UPDATE_FILE_NAME);

  SYSLOG_I("CHECK");
  if (LiteDiskIsInit() == false)
  {
    SYSLOG_W("DISK NOT INIT");
    return UPDATE_RESULT_MISSING;
  }
  if(!f)
  {
    SYSLOG_W("FILE NOT OPEN");
    return UPDATE_RESULT_FAIL;    
  }    
  LiteDiskFileClear(f); // Очищаем файл
  return UPDATE_RESULT_OK;
}
