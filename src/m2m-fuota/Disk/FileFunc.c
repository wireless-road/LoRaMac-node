//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include <string.h>

#include "FileFunc.h"
#include "crc.h"


#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "FILE:"
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

//*************************** ***************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Get info  file
//******************************************************************************
bool FileGetInfo(LT_FILE *f, uint32_t StartPos, uint32_t Size, INFO_STRUCT *Info)
{
  VER_STRUCT tmp;

  memset(Info, 0, sizeof(INFO_STRUCT));
  if (LiteDiskIsInit() == false) return false;  
  SYSLOG_I("File:%s. GET INFO", f->Name);
  for(uint32_t position = StartPos; position < Size; position += 4)
  {
    LiteDiskFileRead(f, position, sizeof(VER_STRUCT), (uint8_t*)&tmp);
    if((tmp.num == M_NUN) && (strncmp((char*)tmp.str, M_STR, 8) == 0))
    {
      *Info = tmp.info;
      SYSLOG_I("File:%s. INFO FOUND. DevID = %d, Ver:%d.%d.%d", f->Name, Info->dev_id, Info->version[0], Info->version[1], Info->version[2]);
      return true;
    }
  }
  SYSLOG_I("File:%s. INFO NOT FOUND", f->Name);
  return false;
}

//******************************************************************************
// Check crc file
//******************************************************************************
bool FileCheckCrcFile(LT_FILE *f, uint32_t Size)
{
  uint32_t CrcCalcIar = 0;
  uint32_t CrcCalcGcc = 0;
  uint32_t CrcRead;
  uint8_t Buff[256];
  uint32_t AmountRead;
  int Result;
  
  if (LiteDiskIsInit() == false) return false;
  SYSLOG_I("File:%s. CHECK CRC", f->Name);
  for (AmountRead =0; AmountRead < Size; ) 
  {
    Result = LiteDiskFileRead(f, AmountRead, sizeof(Buff), Buff);
    if(Result == sizeof(Buff))
    {
      AmountRead += Result;
      if (AmountRead < Size)
      {
        CrcCalcIar = crc32_iar(CrcCalcIar, Buff, Result);
        CrcCalcGcc = crc32_gcc(CrcCalcGcc, Buff, Result);
      }
      else
      {
        CrcCalcIar = crc32_iar(CrcCalcIar, Buff, (Result - 4));
        CrcCalcGcc = crc32_gcc(CrcCalcGcc, Buff, (Result - 4));
	CrcRead = (uint32_t)(Buff[255] << 24) + (uint32_t)(Buff[254] << 16) + (uint32_t)(Buff[253] << 8) + (uint32_t)(Buff[252] << 0);
      }    
    }
    else
    {
      SYSLOG_E("File:%s. READ ERR=%d", f->Name, Result);
      return false;
    }
  }
  SYSLOG_I("File:%s. CRC_CALC_GCC=0x%08X, CRC_CALC_IAR=0x%08X, CRC_READ=0x%08X", f->Name, CrcCalcGcc, CrcCalcIar, CrcRead);
  if ((CrcCalcGcc != CrcRead) && (CrcCalcIar != CrcRead))
  {
    SYSLOG_E("File:%s. ERROR CRC", f->Name);
    return false;
  }
  SYSLOG_I("File:%s. CRC OK", f->Name);
  return true;
}

