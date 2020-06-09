//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include "LiteDisk.h"
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
static LT_FILE *FileTable = NULL;
static isInit = false;

//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

static void FindFileRecord(uint16_t FileID, LT_FILE *File)
{
	memset(File, 0 ,sizeof(LT_FILE));

	int CountTable = 0;
	while (FileTable[CountTable].Id != NULL_FILE_ID)
	{
		if (FileTable[CountTable].Id == FileID)
		{
			*File = FileTable[CountTable];
			break;
		}
		CountTable++;
	}
};

//******************************************************************************
// Public Functions
//******************************************************************************

//******************************************************************************
// Инициализация диска и упрощенной структуры хранения
//******************************************************************************
DRESULT LiteDiskInit(LT_DISK *Disk, void *DiskInitStr, LT_FILE *Table)
{
	DRESULT Result;

	/* Инициализируем микпросхему, если требуеться */
	if (Disk->disk_initialize)
	{
		Result = Disk->disk_initialize(DiskInitStr);
		if (Result != DRESULT_OK)
		{
			return Result;
		}
	}
	IoDisk = Disk;
	FileTable =	Table;
	isInit = true;
	return DRESULT_OK;
}

//******************************************************************************
// Очитка всех секторов принадлежаших "файлу"
//******************************************************************************
int LiteDiskFileClear(uint16_t FileID)
{
	DRESULT Result;
	LT_FILE FILE;

	if ((!IoDisk) || (!IoDisk->disk_sector_erase))
	{
		return DRESULT_NOTRDY;
	}
	/* Ищем данные о записи */
	FindFileRecord(FileID, &FILE);
	if (FILE.Id == NULL_FILE_ID) return (int)DRESULT_PARERR;
	/* Стираем посекторно */
	for (uint16_t SectorCount = FILE.StartSector; SectorCount < (FILE.StartSector + FILE.TotalSectors); SectorCount++)
	{
		Result = IoDisk->disk_sector_erase(SectorCount);
		if (Result != DRESULT_OK)
		{
			return -1;
		}
	}
	return (FILE.TotalSectors * IoDisk->SectorSize);
}

//******************************************************************************
// Запись данных в "файл"
//******************************************************************************
int LiteDiskFileWrite(uint16_t FileID, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
	DRESULT Result;
	LT_FILE FILE;
	int Writed = 0;
	uint32_t Sector, SectorOffs, SectorWriteSize, SectorWrited;
	uint32_t tWrited;

	if ((!IoDisk) || (!IoDisk->disk_sector_write))
	{
		return DRESULT_WRPRT;
	}
	/* Ищем данные о записи */
	FindFileRecord(FileID, &FILE);
	if (FILE.Id == NULL_FILE_ID) return (int)DRESULT_PARERR;
	/* Пишем */
	// Начальная инициализация переменных
	tWrited = 0;
	Sector = (Offs/ IoDisk->SectorSize);
	SectorOffs = (Offs % IoDisk->SectorSize);
	if ((IoDisk->SectorSize - SectorOffs) < Size) SectorWriteSize = (IoDisk->SectorSize - SectorOffs);
	else SectorWriteSize = Size;
	while ( (tWrited < Size) && (Sector < FILE.TotalSectors))
	{
		SectorWrited = IoDisk->disk_sector_write(&Data[tWrited], (Sector + FILE.StartSector), SectorOffs, SectorWriteSize);
		if (SectorWrited < 0) return SectorWrited;
		tWrited += SectorWrited;
		Sector++;
		SectorOffs = 0;
		if ((Size - tWrited) < IoDisk->SectorSize) SectorWriteSize = (Size - tWrited);
		else SectorWriteSize = IoDisk->SectorSize;
	}
	return tWrited;
}

//******************************************************************************
// Чтение данных из файла
//******************************************************************************
int LiteDiskFileRead(uint16_t FileID, uint32_t Offs, uint32_t Size, uint8_t *Data)
{
	DRESULT Result;
	LT_FILE FILE;
	int Writed = 0;
	uint32_t Sector, SectorOffs, SectorReadSize, SectorRead;
	uint32_t tRead;

	if ((!IoDisk) || (!IoDisk->disk_sector_read))
	{
		return DRESULT_NOTRDY;
	}
	/* Ищем данные о записи */
	FindFileRecord(FileID, &FILE);
	if (FILE.Id == NULL_FILE_ID) return (int)DRESULT_PARERR;
	/* Пишем */
	// Начальная инициализация переменных
	tRead = 0;
	Sector = (Offs/ IoDisk->SectorSize);
	SectorOffs = (Offs % IoDisk->SectorSize);
	if ((IoDisk->SectorSize - SectorOffs) < Size) SectorReadSize = (IoDisk->SectorSize - SectorOffs);
	else SectorReadSize = Size;
	while ( (tRead < Size) && (Sector < FILE.TotalSectors))
	{
		SectorRead = IoDisk->disk_sector_read(&Data[tRead], (Sector + FILE.StartSector), SectorOffs, SectorReadSize);
		if (SectorRead < 0) return SectorRead;
		tRead += SectorRead;
		Sector++;
		SectorOffs = 0;
		if ((Size - tRead) < IoDisk->SectorSize) SectorReadSize = (Size - tRead);
		else SectorReadSize = IoDisk->SectorSize;
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


