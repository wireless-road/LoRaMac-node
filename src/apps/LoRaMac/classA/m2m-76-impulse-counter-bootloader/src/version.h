#ifndef _VERSION_H_
#define _VERSION_H_

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


#define SOFT_VER 	0,0,1
#define DEV_ID 		1

#endif
