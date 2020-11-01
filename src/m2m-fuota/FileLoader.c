//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include <String.h>
#include "FileLoader.h"

#include "LmHandler.h"
#include "LmhpFragmentation.h"
#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "Update.h"
#include "crc.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "FLOADER:"
#include "syslog.h"

#if 0
#define TEST_LOAD
#endif



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
static FILE_LOADER_INFO gLoadInfo;

static LT_FILE *fLoad = NULL;
static LT_FILE *fList = NULL;

static size_t pList;

static uint32_t WaitReloadPacket;

//******************************************************************************
// Public Data
//******************************************************************************

uint32_t FileLoaderTaskId;

PROCESS_FUNC FileLoaderFunc =
{
	.Init = FileLoaderStart,
	.Proc = FileLoaderProc,
	.Stop = FileLoaderStop,
	.IsNeedRun = FileLoaderIsRun,
};

//******************************************************************************
// Private Functions
//******************************************************************************


static uint8_t FragDecoderBegin( uint32_t size  )
{
  if(fLoad == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if (size > fLoad->Size)
  {
    SYSLOG_E("ERROR SIZE");
    return -1;
  }
  LiteDiskFileClear(fLoad); // ������� ����
  LiteDiskFileClear(fList); // ������� ����  
  memset(&gLoadInfo, 0, sizeof(gLoadInfo));
  pList = 0;
  gStat = FILE_LOADER_PROC;
  return 0; // Success
}

static uint8_t FragDecoderWrite( uint32_t addr, uint8_t *data, uint32_t size )
{
  int Len = 0;
  FILE_PART_DESCRIPTION Part = {0, 0};
  
  SYSLOG_D("TMP WRITE. Offs = %d, Size = %d", addr, size);
  if( fLoad == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if ((addr + size) > fLoad->Size)
  {
    SYSLOG_E("ERROR OFFSET + SIZE");
    return -1;
  }  
  Len = LiteDiskFileWrite(fLoad, addr, size, data);
  if (Len <= 0)
  {
    SYSLOG_E("ERROR WRITE FILE");
    return -1;    
  }
  if (fList)
  {
    Part.Addr = addr;
    Part.Size = size;
    Len = LiteDiskFileWrite(fList, pList * sizeof(FILE_PART_DESCRIPTION), sizeof(FILE_PART_DESCRIPTION), (uint8_t*)&Part);
    if (Len != sizeof(FILE_PART_DESCRIPTION))
    {
      SYSLOG_E("ERROR WRITE LIST FILE");
    }
    else
    {
      SYSLOG_I("WRITE LIST FILE OK. Part.Addr = %d, Part.Size = %d", Part.Addr, Part.Size);
    }
    pList++;
  }
  SYSLOG_D("TMP FILE WRITED");
  return 0; // Success
}

static uint8_t FragDecoderRead( uint32_t addr, uint8_t *data, uint32_t size )
{
  int Len = 0;
  
  if( fLoad == NULL )
  {
    SYSLOG_E("FILE NOT OPEN");
    return -1;
  }
  if ((addr + size) > fLoad->Size)
  {
    SYSLOG_E("ERROR OFFSET + SIZE");
    return -1;
  }  
  Len = LiteDiskFileRead(fLoad, addr, size, data);
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
  gLoadInfo.Size = size;
  gLoadInfo.SizeList = pList;
  SYSLOG_D("FINISHED. STATUS %d, SIZE = %d", status, size);  
  gStat = FILE_LOADER_ANALYSIS;
}

static bool MoveTmpFile()
{
    int Len = 0;
    uint8_t Buff[256];
    FILE_PART_DESCRIPTION Part;
    LT_FILE *fUpdate;
    INFO_STRUCT InfoUpdate;
    UPDATE_RESULT CheckResult;
    
    fUpdate = LiteDiskFileOpen(UPDATE_FILE_NAME);

    if(!fUpdate)
    {
      SYSLOG_E("Not open update file");
      return false;
    }      
    SYSLOG_I("Copy tmp block to update file");
    /* ��������� ���������� ����� �� ���������� ����� � ���� Update*/
    for (size_t i = 0; i < gLoadInfo.SizeList; i++)
    {
        /* ������ ���������� � ����������� ������ */
        Len = LiteDiskFileRead(fList, i * sizeof(FILE_PART_DESCRIPTION), sizeof(FILE_PART_DESCRIPTION), (uint8_t*)&Part);
        if (Len != sizeof(FILE_PART_DESCRIPTION))
        {
          SYSLOG_E("Error read list file. Part = %d", i);
          return false;
        }
        SYSLOG_I("Part[%d] Addr = %d, Size = %d", i, Part.Addr, Part.Size);
        Len = LiteDiskFileRead(fLoad, Part.Addr, Part.Size, Buff);
        if (Len != Part.Size)
        {
          SYSLOG_E("Error read tmp file. Addr = %d, Size = %d", Part.Addr, Part.Size);
          return false;
        }       
        Len = LiteDiskFileReWrite(fUpdate, Part.Addr, Part.Size, Buff);
        if (Len != Part.Size)
        {
          SYSLOG_E("Error write update file. Addr = %d, Size = %d", Part.Addr, Part.Size);
          return false;
        }               
    }
    SYSLOG_I("Copy compleate. Check");
    /* ��������� �������� �����*/
    CheckResult = UpdateCheck(&InfoUpdate);
    SYSLOG_I("Result = %d, DevID=%d, Version: %d.%d.%d", CheckResult, InfoUpdate.dev_id, InfoUpdate.version[0], InfoUpdate.version[1], InfoUpdate.version[2]);
    if (CheckResult != UPDATE_RESULT_OK)
    {
      return false;
    }
    return true;
}

static bool FileLoaderUplink(void)
{
	FILE_LOADER_INFO Info;
	FILE_LOADER_STAT Stat;
	uint8_t Data[16];
	uint8_t pData = 0;

	Stat = FileLoaderGetStat(&Info);

	SYSLOG_I("Load info send. Stat=%d, Size = %d", Stat, Info.Size);
	Data[pData++] = 0x05; // FragDataBlockAuthReq
	Data[pData++] = (uint8_t)(Stat);
	Data[pData++] = Info.Size & 0x000000FF;
	Data[pData++] = ( Info.Size >> 8 ) & 0x000000FF;
	Data[pData++] = ( Info.Size >> 16 ) & 0x000000FF;
	Data[pData++] = ( Info.Size >> 24 ) & 0x000000FF;
	return LoRaWanSend(201, pData, Data);
}

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Start update task
//******************************************************************************
void FileLoaderStart(void)
{
  fLoad = LiteDiskFileOpen(TMP_FILE_NAME);
  fList = LiteDiskFileOpen(LIST_FILE_NAME);
  if (!fLoad)
  {
    SYSLOG_E("NOT OPEN FILE");
    gStat = FILE_LOADER_ERROR;
    return;
  }
  pList = 0;
  memset(&gLoadInfo, 0, sizeof(gLoadInfo));
  gStat = FILE_LOADER_WAIT;
  LmHandlerPackageRegister( PACKAGE_ID_FRAGMENTATION, &gFragmentationParams );
  SYSLOG_I("TASK STARTED");
}

#ifdef TEST_LOAD
uint32_t TestLoadOffs = 0;
uint8_t BuffTest[115];
uint8_t BuffRead[256];

static bool CmpBuff(uint8_t *Buff1, uint8_t *Buff2, size_t Size)
{
	for(int i = 0; i < Size; i++)
	{
		if (Buff1[i] != Buff2[i])
		{
			//SYSDUMP_E("BUFF1",Buff1,Size);
			//SYSDUMP_E("BUFF2",Buff2,Size);
			return false;
		}
	}
	return true;
}
#endif
//******************************************************************************
// Update task time proc
//******************************************************************************
void FileLoaderProc(void)
{
  uint32_t Size;
  switch(gStat)
  {
  case FILE_LOADER_ANALYSIS:
    WaitReloadPacket = 0;
    if (MoveTmpFile() == true)
    {
      gStat = FILE_LOADER_SUCCESS;
    }
    else
    {
      gStat = FILE_LOADER_FAIL;
    }
  break;
  case FILE_LOADER_SUCCESS:
	  if (FileLoaderUplink() == true)
	  {
		  WaitReloadPacket++;
	  }
	  if (WaitReloadPacket > 3)
	  {
		  BoardResetMcu();
	  }
  break;
  case FILE_LOADER_WAIT:
  case FILE_LOADER_PROC:
  default:
	  break;
  }

#ifdef TEST_LOAD
  switch (gStat)
  {
  	case FILE_LOADER_WAIT:
  		TestLoadOffs = 0;
  		FragDecoderBegin(APP_SIZE);
  	break;

  	case FILE_LOADER_PROC:
  		if (TestLoadOffs < APP_SIZE)
  		{
  			if ((APP_SIZE - TestLoadOffs) > sizeof(BuffTest)) Size = sizeof(BuffTest); else Size = (APP_SIZE - TestLoadOffs);
  			memset(BuffTest, 0xFF, sizeof(BuffTest));
  			memcpy(BuffTest,(uint8_t*)(APP_START_ADDRESS + TestLoadOffs), Size);
  			FragDecoderWrite(TestLoadOffs, BuffTest, Size);
  			LiteDiskFileRead(fLoad, TestLoadOffs, Size, BuffRead);
  			if (CmpBuff(BuffTest, BuffRead, Size) != true)
  			{
  				SYSLOG_E("Error cmp");
  			}
  			else
  			{
  				SYSLOG_I("Cmp OK");
  			}
  			TestLoadOffs += sizeof(BuffTest);
  		}
  		else
  		{
  			OnFragDone(0, TestLoadOffs);
  		}
  	break;
  }
#endif
}

//******************************************************************************
// Stop update task
//******************************************************************************
void FileLoaderStop(void)
{
  
}

bool FileLoaderIsRun(void)
{
	if ((gStat == FILE_LOADER_WAIT) || (gStat == FILE_LOADER_ERROR)) return false;
	return true;
}

//******************************************************************************
// Get upload stat
//******************************************************************************
FILE_LOADER_STAT FileLoaderGetStat(FILE_LOADER_INFO *Info)
{
  *Info = gLoadInfo;
  if (gStat == FILE_LOADER_SUCCESS)
  {
	  WaitReloadPacket++;
  }
  return gStat;
}



