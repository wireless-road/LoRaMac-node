//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
#include <string.h>

#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "stdint.h"
#include "stddef.h"

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

static LT_DISK *IoDisk = NULL;
static bool isInit = false;
static LT_FILE gFiles[MAX_FILES];

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
// Инициализация диска и упрощенной структуры хранения
//******************************************************************************
DRESULT LiteDiskInit(LT_DISK *Disk, void *DiskInitStr, LT_FILE_DEFS *Table)
{
  DRESULT Result;
  uint32_t Sector = 0;
  /* Инициализируем микпросхему, если требуеться */
  if (Disk->disk_initialize)
  {
    Result = (DRESULT)Disk->disk_initialize(DiskInitStr);
      if (Result != DRESULT_OK)
      {
        return Result;
      }
  }
  memset(&gFiles, 0, sizeof(gFiles));
  for (int i = 0; ((i < MAX_FILES) || (Table[i].Name != NULL)); i++)
  {
    gFiles[i].Name = Table[i].Name;
    gFiles[i].Size = Table[i].Size;
    gFiles[i].StartSector = Sector;
    gFiles[i].AmountSectors = ((gFiles[i].Size + Disk->SectorSize -1) / Disk->SectorSize);
    Sector += gFiles[i].AmountSectors;
  }
  
  IoDisk = Disk;
  isInit = true;
  return DRESULT_OK;
}

//******************************************************************************
// Открытие файла
//******************************************************************************
LT_FILE *LiteDiskFileOpen(char *Name)
{
  if (!Name) return NULL;
  for (int i = 0; (i < MAX_FILES); i++)
  {
    if (strcmp(Name, gFiles[i].Name) == 0)
    {
      return &gFiles[i];
    }
  }
  return NULL;
}

//******************************************************************************
// Очитка всех секторов принадлежаших "файлу"
//******************************************************************************
int LiteDiskFileClear(LT_FILE *f)
{
  DRESULT Result;

  if ((!IoDisk) || (!IoDisk->disk_sector_erase))
  {
    return DRESULT_NOTRDY;
  }
  if (!f) return DRESULT_PARERR;
  /* Стираем посекторно */
  for (uint32_t SectorCount = f->StartSector; SectorCount < (f->StartSector + f->AmountSectors); SectorCount++)
  {
    Result = (DRESULT)IoDisk->disk_sector_erase(SectorCount);
    if (Result != DRESULT_OK)
    {
      return Result;
    }
  }
  return (f->AmountSectors * IoDisk->SectorSize);
}

//******************************************************************************
// Запись данных в "файл"
//******************************************************************************
int LiteDiskFileWrite(LT_FILE *f, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
  uint32_t Sector, SectorOffs, SectorWriteSize;
  uint32_t tWrited;
  int SectorWrited;

  if ((!IoDisk) || (!IoDisk->disk_sector_write))
  {
    return DRESULT_WRPRT;
  }
   if (!f) return DRESULT_PARERR;
  /* Пишем */
  // Начальная инициализация переменных
  tWrited = 0;
  Sector = (Offs/ IoDisk->SectorSize);
  SectorOffs = (Offs % IoDisk->SectorSize);
  if ((IoDisk->SectorSize - SectorOffs) < Size) SectorWriteSize = (IoDisk->SectorSize - SectorOffs);
  else SectorWriteSize = Size;
  while ( (tWrited < Size) && (f->AmountSectors))
  {
    SectorWrited = IoDisk->disk_sector_write((Sector + f->StartSector), SectorOffs, SectorWriteSize, &Data[tWrited]);
    if (SectorWrited < 0) return SectorWrited;
    tWrited += SectorWrited;
    Sector++;
    SectorOffs = 0;
    if ((Size - tWrited) < IoDisk->SectorSize) SectorWriteSize = (Size - tWrited); else SectorWriteSize = IoDisk->SectorSize;
  }
  return tWrited;
}

//******************************************************************************
// Чтение данных из файла
//******************************************************************************
int LiteDiskFileRead(LT_FILE *f, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
  uint32_t Sector, SectorOffs, SectorReadSize;
  uint32_t tRead;
  int SectorRead;

  if ((!IoDisk) || (!IoDisk->disk_sector_read))
  {
    return DRESULT_NOTRDY;
  }
  if (!f) return DRESULT_PARERR;
  /* Пишем */
  // Начальная инициализация переменных
  tRead = 0;
  Sector = (Offs/ IoDisk->SectorSize);
  SectorOffs = (Offs % IoDisk->SectorSize);
  if ((IoDisk->SectorSize - SectorOffs) < Size) SectorReadSize = (IoDisk->SectorSize - SectorOffs);
  else SectorReadSize = Size;
  while ( (tRead < Size) && (Sector < f->AmountSectors))
  {
    SectorRead = IoDisk->disk_sector_read((Sector + f->StartSector), SectorOffs, SectorReadSize, &Data[tRead]);
    if (SectorRead < 0) return SectorRead;
    tRead += SectorRead;
    Sector++;
    SectorOffs = 0;
    if ((Size - tRead) < IoDisk->SectorSize) SectorReadSize = (Size - tRead); else SectorReadSize = IoDisk->SectorSize;
  }
  return tRead;
}

//******************************************************************************
// Проверка, что диск проинициализтрован
//******************************************************************************
bool LiteDiskIsInit(void)
{
  return isInit;
}


