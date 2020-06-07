#include "version.h"


const VER_STRUCT __ver_struct __attribute__ ((section(".version"))) =
{
  .num = M_NUN,
  .str = M_STR,
  .info = 
  {
    .dev_id  = DEV_ID,
#ifdef BOOTLOADER
    .version = { BOOT_VER },
#else
	.version = { APP_VER },
#endif
  }
};


VER_RESULT VersionRead(uint32_t StartAddr, uint32_t SizePart, INFO_STRUCT *INFO)
{
	VER_STRUCT *tmp;

	if (!INFO) return VER_NOT;
	memset(INFO, 0, sizeof(INFO_STRUCT)); // Очищаем перед выводом
	for(uint32_t addr = StartAddr; addr < (StartAddr + SizePart) ; addr++)
	{
	  tmp = (VER_STRUCT*)addr;
	  if ((tmp->num == M_NUN) && (strncmp((char*)tmp->str, M_STR, 8) == 0))
	  {
	    INFO->dev_id = tmp->info.dev_id;
	    INFO->version[0] = tmp->info.version[0];
	    INFO->version[1] = tmp->info.version[1];
	    INFO->version[2] = tmp->info.version[2];
	    return VER_FOUND;
	  }
	}
	return VER_NOT;
}
