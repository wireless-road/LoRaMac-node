//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "FileLoader.h"

#include "LmHandler.h"
#include "LmhpFragmentation.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "crc.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "FLOADER:"
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

static uint8_t FragDecoderBegin( uint32_t size );
static uint8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size );
static uint8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size );
static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost );
static void OnFragDone( int32_t status, uint32_t size );
//******************************************************************************
// Private Data
//******************************************************************************

static LmhpFragmentationParams_t gFragmentationParams =
{
    .DecoderCallbacks =
    {
        .FragDecoderBegin = FragDecoderBegin,
        .FragDecoderWrite = FragDecoderWrite,
        .FragDecoderRead = FragDecoderRead,
    },
    .OnProgress = OnFragProgress,
    .OnDone = OnFragDone
};


static volatile FILE_LOADER_STAT gStat = FILE_LOADER_WAIT;

static LT_FILE *gUpdateFile = NULL;

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************


static uint8_t FragDecoderBegin( uint32_t size  )
{
  int Result;
  
  if( gUpdateFile == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if (size > gUpdateFile->Size)
  {
    SYSLOG_E("ERROR SIZE");
    return -1;
  }
  Result = LiteDiskFileClear(gUpdateFile); // Очищаем файл
  if(Result < 0)
  {
    SYSLOG_E("ERROR CLEAR FILE");
    return -1;
  }    
  gStat = FILE_LOADER_PROC;
  return 0; // Success
}

static uint8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size )
{
  int Len = 0;
  
  if( gUpdateFile == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if ((addr + size) > gUpdateFile->Size)
  {
    SYSLOG_E("ERROR OFFSET + SIZE");
    return -1;
  }  
  Len = LiteDiskFileWrite(gUpdateFile, addr, size, data);
  if (Len <= 0)
  {
    SYSLOG_E("ERROR WRITE FILE");
    return -1;    
  }
  SYSLOG_D("WRITED. Offs = %d, Size = %d", addr, size);
  return 0; // Success
}

static uint8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size )
{
  int Len = 0;
  
  if( gUpdateFile == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if ((addr + size) > gUpdateFile->Size)
  {
    SYSLOG_E("ERROR OFFSET + SIZE");
    return -1;
  }  
  Len = LiteDiskFileRead(gUpdateFile, addr, size, data);
  if (Len <= 0)
  {
    SYSLOG_E("ERROR READ FILE");
    return -1;    
  }
  SYSLOG_D("REATED. Offs = %d, Size = %d", addr, size);
  return 0; // Success
}

static void OnFragProgress( uint16_t fragCounter, uint16_t fragNb, uint8_t fragSize, uint16_t fragNbLost )
{
  SYSLOG_D("RECEIVED %d / %d Fragments %d / %d Bytes. LOST %d Fragments", fragCounter, fragNb, fragCounter * fragSize, fragNb * fragSize, fragNbLost);
}


static void OnFragDone( int32_t status, uint32_t size )
{
	gStat = FILE_LOADER_ANALYSIS;
    SYSLOG_D("FINISHED. STATUS %d, SIZE = %d", status, size);
}

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Start update task
//******************************************************************************
bool FileLoaderStart(void)
{
  gUpdateFile = LiteDiskFileOpen(TMP_FILE_NAME);
  if (!gUpdateFile)
  {
    SYSLOG_E("NOT OPEN FILE");
    return false;
  }

  gStat = FILE_LOADER_WAIT;
  LmHandlerPackageRegister( PACKAGE_ID_FRAGMENTATION, &gFragmentationParams );
  SYSLOG_I("TASK STARTED");
  return true;
}

//******************************************************************************
// Update task time proc
//******************************************************************************
void FileLoaderProc(void)
{
  switch(gStat)
  {
  case FILE_LOADER_WAIT:
  case FILE_LOADER_PROC:
  default:
	  break;
  }
}

//******************************************************************************
// Stop update task
//******************************************************************************
void FileLoaderStop(void)
{
  
}


//******************************************************************************
// Get upload stat
//******************************************************************************
FILE_LOADER_STAT FileLoaderGetStat(FILE_LOADER_INFO *Info)
{
  memset(Info, 0, sizeof(FILE_LOADER_INFO));
  return gStat;
}




