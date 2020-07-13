//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "UpdateTask.h"
#include "Update.h"

#include "LmHandler.h"
#include "LmhpFragmentation.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "UPDATE:"
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

/*
 * Indicates if the file transfer is done
 */
static volatile bool gIsFileTransferDone = false;

/*
 *  Received file computed CRC32
 */
static volatile uint32_t gFileRxCrc = 0;

static LT_FILE *gUpdateFile = NULL;

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

static uint32_t UpdateCrc( uint32_t Size )
{
  uint8_t Data;
  uint32_t Len;
    // The CRC calculation follows CCITT - 0x04C11DB7
    const uint32_t reversedPolynom = 0xEDB88320;

    // CRC initial value
    uint32_t crc = 0xFFFFFFFF;

    if( gUpdateFile == NULL )
    {
        return 0;
    }

    for( uint32_t i = 0; i < Size; ++i )
    {
        Len = LiteDiskFileRead(gUpdateFile, i, 1, &Data);
        if (Len <= 0)
        {
          return 0;
        }
        crc ^= ( uint32_t )Data;
        for( uint16_t i = 0; i < 8; i++ )
        {
            crc = ( crc >> 1 ) ^ ( reversedPolynom & ~( ( crc & 0x01 ) - 1 ) );
        }
    }

    return ~crc;
}


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
    //gFileRxCrc = UpdateCrc(size);
    gIsFileTransferDone = true;
    SYSLOG_D("FINISHED. STATUS %d, CRC = 0x%08X", status, gFileRxCrc);
}

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Start update task
//******************************************************************************
bool UpdateTaskStart(void)
{
  gUpdateFile = LiteDiskFileOpen(UPDATE_FILE_NAME);
  if (!gUpdateFile)
  {
    SYSLOG_E("NOT OPEN FILE");
    return false;
  }
  gFileRxCrc = 0;
  gIsFileTransferDone = false;
  LmHandlerPackageRegister( PACKAGE_ID_FRAGMENTATION, &gFragmentationParams );
  SYSLOG_I("TASK STARTED");
  return true;
}

//******************************************************************************
// Update task time proc
//******************************************************************************
void UpdateTaskProc(void)
{
  
}

//******************************************************************************
// Stop update task
//******************************************************************************
void UpdateTaskStop(void)
{
  
}

// Временная функция
bool UpdateTaskIsDone(uint32_t *crc)
{
  if (gIsFileTransferDone == false)
  {
    *crc = 0;
    return false;
  }
  *crc = gFileRxCrc;
  return true;
}




