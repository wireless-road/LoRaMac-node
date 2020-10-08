//******************************************************************************
//
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************
   
#include "config.h"
#include <string.h>

#include "LiteDisk.h"
#include "LiteDiskDefs.h"
#include "crc.h"

#define LOG_LEVEL   MAX_LOG_LEVEL_DEBUG
#define LOG_MODULE  "CONF:"
#include "syslog.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define CHECK_NUN 0xA5A5

//******************************************************************************
// Private Types
//******************************************************************************
typedef struct _CONF_RECORD
{
	uint16_t CheckNum;
	uint16_t Id;
	uint16_t SizeRecord;
	uint16_t SizeConf;
} __attribute__((__packed__ )) CONF_RECORD;

typedef struct _CONF_HEADER
{
  uint16_t  CheckNum;
  uint8_t 	CountRecords;
} __attribute__((__packed__ )) CONF_HEADER;
//******************************************************************************
// Private Function Prototypes
//******************************************************************************

//******************************************************************************
// Private Data
//******************************************************************************
static LT_FILE *fConfig = NULL;
static CONF_HEADER gHeader;
static bool flCorrect = false;
//******************************************************************************
// Public Data
//******************************************************************************

//******************************************************************************
// Private Functions
//******************************************************************************

//******************************************************************************
// Public Functions
//******************************************************************************
RESULT_CONF ConfigFileOpen(void)
{

	int Len;

	flCorrect = false;
	fConfig = LiteDiskFileOpen(CONF_FILE_NAME); // Пытаемся открыть файл
	if (fConfig == NULL)
	{
		SYSLOG_E("Error open file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	// Читаем заголовок
	Len = LiteDiskFileRead(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader);
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error read header");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	// Проверяем файл и при необходимости пересоздаем его
	if (gHeader.CheckNum == CHECK_NUN)
	{
		SYSLOG_I("Config file OK");
		flCorrect = true;
		return CONF_OK;
	}
	SYSLOG_E("Error header");
	return CONF_ERRROR; // Выходим с ошибкой
}

RESULT_CONF ConfigFileCreate(void)
{
	int Len;
	uint8_t Buff[64];

	flCorrect = false;
	fConfig = LiteDiskFileOpen(CONF_FILE_NAME); // Пытаемся открыть файл
	if (!fConfig)
	{
		SYSLOG_E("Error open file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	Len = LiteDiskFileClear(fConfig); // Очищаем файл
	if (Len != CONF_FILE_SIZE)
	{
		SYSLOG_E("Error clear file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	memset(Buff, 0, 64);
	for (int i = 0; i < CONF_FILE_SIZE; )
	{
		Len = LiteDiskFileWrite(fConfig, i, 64, Buff);
		if (Len != 64)
		{
			SYSLOG_E("Error fill file");
			return CONF_ERRROR; // Выходим с ошибкой
		}
		i += Len;
	}
	// Пишем заголовок
	gHeader.CheckNum = CHECK_NUN;
	gHeader.CountRecords = 0;
	Len = LiteDiskFileReWrite(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader); // Очищаем файл
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error write file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	return CONF_OK;
}

RESULT_CONF ConfigPartOpen(ID_CONF Id, uint32_t *Indx)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs;

	Offs = sizeof(CONF_HEADER);
	if ((!fConfig) || (flCorrect == false) || (Id >= ID_CONF_COUNT))
	{
		SYSLOG_E("Error file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	// Ищем запись
	for (int i = 0; i < gHeader.CountRecords; i++)
	{
		Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
		if ((Len == sizeof(CONF_RECORD)) && (Record.Id == Id) && (Record.CheckNum == CHECK_NUN))
		{
			*Indx = Offs;
			return CONF_OK;
		}
		Offs += (sizeof(CONF_RECORD) + Record.SizeRecord);
	}
	SYSLOG_W("Record:%d not found", Id);
	return CONF_NOT;
}

RESULT_CONF ConfigPartCreate(ID_CONF Id, size_t Size)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs = sizeof(CONF_HEADER);

	if ((!fConfig) || (flCorrect == false) || (Id >= ID_CONF_COUNT))
	{
		SYSLOG_E("Error file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	for (int i = 0; i < gHeader.CountRecords; i++)
	{
		Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
		Offs += ((sizeof(CONF_RECORD) + Record.SizeRecord));
	}
	if ((Size + sizeof(CONF_RECORD)) > (CONF_FILE_SIZE - Offs))
	{
		SYSLOG_E("Not place");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	Record.CheckNum = CHECK_NUN;
	Record.Id = Id;
	Record.SizeRecord = Size;
	Record.SizeConf = 0;
	SYSLOG_I("Create:Record.SizeConf=%d,Record.SizeRecord=%d,Record.Id=%d", Record.SizeConf, Record.SizeRecord, Record.Id);
	Len = LiteDiskFileReWrite(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record); // Очищаем файл
	if (Len != sizeof(CONF_RECORD))
	{
		SYSLOG_E("Error write record header");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	// Пишем заголовок
	gHeader.CountRecords += 1;
	Len = LiteDiskFileReWrite(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader); // Очищаем файл
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error write file");
		return CONF_ERRROR; // Выходим с ошибкой
	}
	return CONF_OK;
}


int ConfigPartRead(uint32_t Indx, size_t size, void *Conf)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs;
	uint32_t ReadSize;


	Offs = Indx;
	if ((!fConfig) || (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return -1; // Выходим с ошибкой
	}
	Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
	if (Len != sizeof(CONF_RECORD) || (size > Record.SizeRecord))
	{

		SYSLOG_E("Error read record. Id = %d.SizePart = %d, SizeConf = %d", Record.Id, Record.SizeRecord, Record.SizeConf);
		return -1; // Выходим с ошибкой
	}
	if (size > Record.SizeConf) ReadSize = Record.SizeConf; else ReadSize = size;
	Offs += sizeof(CONF_RECORD);
	return LiteDiskFileRead(fConfig, Offs, ReadSize, (uint8_t*)Conf);
}

int ConfigPartWrite(uint32_t Indx, size_t size, void *Conf)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs;


	Offs = Indx;
	if ((!fConfig) || (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return -1; // Выходим с ошибкой
	}
	Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
	if (Len != sizeof(CONF_RECORD) || (size > Record.SizeRecord))
	{
		SYSLOG_E("Error write record. Id = %d.SizePart = %d, SizeConf = %d", Record.Id, Record.SizeRecord, Record.SizeConf);
		return -1; // Выходим с ошибкой
	}
	// Пишем новый заголовок записи
	Record.SizeConf = size;
	Len = LiteDiskFileReWrite(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record); // Очищаем файл
	if (Len != sizeof(CONF_RECORD))
	{
		SYSLOG_E("Error write record header");
		return -1; // Выходим с ошибкой
	}
	Offs += sizeof(CONF_RECORD);
	return LiteDiskFileReWrite(fConfig, Offs, size, (uint8_t*)Conf);
}


