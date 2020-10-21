#ifndef _VERSION_H_
#define _VERSION_H_

#include <stdint.h>

#define M_NUN 0xAA55
#define M_STR "VERSION"

typedef enum
{
	VER_NOT,
	VER_FOUND,
} VER_RESULT;

typedef unsigned char VERSION[3];

typedef struct _INFO_STRUCT
{
  unsigned short dev_id;    	//ID устроства
  VERSION        version;   	//Версия прошивки
} __attribute__((__packed__ )) INFO_STRUCT;


typedef struct _VER_STRUCT
{
  unsigned short  num;
  unsigned char   str[8];
  INFO_STRUCT     info;
  unsigned char   dummy;
} __attribute__((__packed__ )) VER_STRUCT;

#ifdef BOOTLOADER
#define BOOT_VER 	0,0,2
#else
#define APP_VER 	0,2,0
#endif

#ifdef IMPULSE_COUNTER
#define DEV_ID 		1
#endif


#ifdef DECAST_WATER
#define DEV_ID 		2
#endif

VER_RESULT VersionRead(uint32_t StartAddr, uint32_t SizePart, INFO_STRUCT *INFO);

#endif
