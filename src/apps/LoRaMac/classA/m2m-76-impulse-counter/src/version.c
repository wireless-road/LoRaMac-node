#include "version.h"

#define M_NUN 0xAA55
#define M_STR "VERSION"

const VER_STRUCT __ver_struct __attribute__ ((section(".version"))) =
{
  .num = M_NUN,
  .str = M_STR,
  .info = 
  {
    .dev_id  = DEV_ID,
    .version = { SOFT_VER },
  }
};
