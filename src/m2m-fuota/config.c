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
	fConfig = LiteDiskFileOpen(CONF_FILE_NAME); // �������� ������� ����
	if (!fConfig)
	{
		SYSLOG_E("Error open file");
		return CONF_ERRROR; // ������� � �������
	}
	// ������ ���������
	Len = LiteDiskFileRead(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader);
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error read header");
		return CONF_ERRROR; // ������� � �������
	}
	// ��������� ���� � ��� ������������� ����������� ���
	if (gHeader.CheckNum == CHECK_NUN)
	{
		SYSLOG_I("Config file OK");
		flCorrect = true;
		return CONF_OK;
	}
	SYSLOG_E("Error header");
	return CONF_ERRROR; // ������� � �������
}

RESULT_CONF ConfigFileCreate(void)
{
	int Len;
	uint8_t Buff[64];

	if (!fConfig)
	{
		SYSLOG_E("Error open file");
		return CONF_ERRROR; // ������� � �������
	}
	Len = LiteDiskFileClear(fConfig); // ������� ����
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error clear file");
		return CONF_ERRROR; // ������� � �������
	}
	memset(Buff, 0, 64);
	for (int i = 0; i < CONF_FILE_SIZE; )
	{
		Len = LiteDiskFileWrite(fConfig, i, 64, Buff);
		if (Len != sizeof(CONF_HEADER))
		{
			SYSLOG_E("Error clear file");
			return CONF_ERRROR; // ������� � �������
		}
		i += Len;
	}
	// ����� ���������
	gHeader.CheckNum = CHECK_NUN;
	gHeader.CountRecords = 0;
	Len = LiteDiskFileReWrite(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader); // ������� ����
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error write file");
		return CONF_ERRROR; // ������� � �������
	}
	return CONF_OK;
}

RESULT_CONF ConfigPartOpen(uint16_t Id, uint32_t *Indx)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs = sizeof(CONF_HEADER);

	if ((!fConfig) && (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return CONF_ERRROR; // ������� � �������
	}
	// ���� ������
	for (int i = 0; i < gHeader.CountRecords; i++)
	{
		Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
		if ((Len == sizeof(CONF_RECORD)) && (Record.Id == Id))
		{
			*Indx = Offs;
			return CONF_OK;
		}
		Offs += (sizeof(CONF_RECORD) + Record.SizeRecord);
	}
	SYSLOG_W("Record:%d not found", Id);
	return CONF_NOT;
}

RESULT_CONF ConfigPartCreate(uint16_t Id, size_t Size, uint32_t *Indx)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs = sizeof(CONF_HEADER);

	if ((!fConfig) && (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return CONF_ERRROR; // ������� � �������
	}
	for (int i = 0; i < gHeader.CountRecords; i++)
	{
		Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
		Offs += ((sizeof(CONF_RECORD) + Record.SizeRecord));
	}
	if ((Size + sizeof(CONF_RECORD)) > (CONF_FILE_SIZE - Offs))
	{
		SYSLOG_E("Not place");
		return CONF_ERRROR; // ������� � �������
	}
	Record.SizeConf = 0;
	Record.SizeRecord = Size;
	Record.Id = Id;
	Len = LiteDiskFileReWrite(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record); // ������� ����
	if (Len != sizeof(CONF_RECORD))
	{
		SYSLOG_E("Error write record header");
		return CONF_ERRROR; // ������� � �������
	}
	// ����� ���������
	gHeader.CountRecords += 1;
	Len = LiteDiskFileReWrite(fConfig, 0, sizeof(CONF_HEADER), (uint8_t*)&gHeader); // ������� ����
	if (Len != sizeof(CONF_HEADER))
	{
		SYSLOG_E("Error write file");
		return CONF_ERRROR; // ������� � �������
	}
	return CONF_OK;
}


int ConfigPartRead(uint32_t Indx, size_t size, void *Conf)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs = sizeof(CONF_HEADER) + Indx;
	uint32_t ReadSize;

	if ((!fConfig) && (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return -1; // ������� � �������
	}
	Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
	if (Len != sizeof(CONF_RECORD) || (size > Record.SizeRecord))
	{
		SYSLOG_E("Error read record");
		return CONF_ERRROR; // ������� � �������
	}
	if (size > Record.SizeConf) ReadSize = Record.SizeConf; else ReadSize = size;
	Offs += sizeof(CONF_RECORD);
	return LiteDiskFileRead(fConfig, Offs, ReadSize, (uint8_t*)Conf);
}

int ConfigPartWrite(uint32_t Indx, size_t size, void *Conf)
{
	int Len;
	CONF_RECORD Record;
	uint32_t Offs = sizeof(CONF_HEADER) + Indx;

	if ((!fConfig) && (flCorrect == false))
	{
		SYSLOG_E("Error file");
		return -1; // ������� � �������
	}
	Len = LiteDiskFileRead(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record);
	if (Len != sizeof(CONF_RECORD) || (size > Record.SizeRecord))
	{
		SYSLOG_E("Error read record");
		return CONF_ERRROR; // ������� � �������
	}
	// ����� ����� ��������� ������
	Record.SizeConf = size;
	Len = LiteDiskFileReWrite(fConfig, Offs, sizeof(CONF_RECORD), (uint8_t*)&Record); // ������� ����
	if (Len != sizeof(CONF_RECORD))
	{
		SYSLOG_E("Error write record header");
		return CONF_ERRROR; // ������� � �������
	}
	Offs += sizeof(CONF_RECORD);
	return LiteDiskFileReWrite(fConfig, Offs, size, (uint8_t*)Conf);
}


