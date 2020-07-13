//******************************************************************************
//Привязка к "железу" драйвера микросхемы flash памяти
//******************************************************************************

//******************************************************************************
// Included Files
//******************************************************************************

#include "mx25l.h"

//******************************************************************************
// Pre-processor Definitions
//******************************************************************************

#define WAIT_FOREVER (uint32_t)0xFFFFFFFF
/* Chip Geometries ******************************************************************/

/* MX25L3233F capacity is 32Mbit  (4096Kbit x 8) =   4Mb (512kb x 8) */

#define MX25L_MX25L3233F_SECTOR_SHIFT  12    /* Sector size 1 << 12 = 4Kb */
#define MX25L_MX25L3233F_NSECTORS      1024
#define MX25L_MX25L3233F_PAGE_SHIFT    8     /* Page size 1 << 8 = 256 */

/* MX25L6433F capacity is 32Mbit  (8192Kbit x 8) =   8Mb (1024kb x 8) */

#define MX25L_MX25L6433F_SECTOR_SHIFT  12    /* Sector size 1 << 12 = 4Kb */
#define MX25L_MX25L6433F_NSECTORS      2048
#define MX25L_MX25L6433F_PAGE_SHIFT    8     /* Page size 1 << 8 = 256 */
   
#ifdef CONFIG_MX25L_SECTOR512                /* Simulate a 512 byte sector */
#define MX25L_SECTOR512_SHIFT           9     /* Sector size 1 << 9 = 512 bytes */
#endif

#define MX25L_ERASED_STATE             0xff   /* State of FLASH when erased */

#define MX25L_CACHE_VALID          (1 << 0)   /* 1=Cache has valid data */
#define MX25L_CACHE_DIRTY          (1 << 1)   /* 1=Cache is dirty */
#define MX25L_CACHE_ERASED         (1 << 2)   /* 1=Backing FLASH is erased */

#define IS_VALID(p)                ((((p)->flags) & MX25L_CACHE_VALID) != 0)
#define IS_DIRTY(p)                ((((p)->flags) & MX25L_CACHE_DIRTY) != 0)
#define IS_ERASED(p)               ((((p)->flags) & MX25L_CACHE_ERASED) != 0)

#define SET_VALID(p)               do { (p)->flags |= MX25L_CACHE_VALID; } while (0)
#define SET_DIRTY(p)               do { (p)->flags |= MX25L_CACHE_DIRTY; } while (0)
#define SET_ERASED(p)              do { (p)->flags |= MX25L_CACHE_ERASED; } while (0)

#define CLR_VALID(p)               do { (p)->flags &= ~MX25L_CACHE_VALID; } while (0)
#define CLR_DIRTY(p)               do { (p)->flags &= ~MX25L_CACHE_DIRTY; } while (0)
#define CLR_ERASED(p)              do { (p)->flags &= ~MX25L_CACHE_ERASED; } while (0)

/* MX25L Instructions *****************************************************************/
/*      Command                    Value      Description               Addr   Data   */
/*                                                                         Dummy      */
#define MX25L_READ                  0x03    /* Read data bytes          3   0   >=1   */
#define MX25L_FAST_READ             0x0b    /* Higher speed read        3   1   >=1   */
#define MX25L_2READ                 0xbb    /* 2 x I/O read command     */
#define MX25L_DREAD                 0x3b    /* 1I / 2O read command     3   1   >=1   */
#define MX25L_4READ                 0xeb    /* 4 x I/O read command     */
#define MX25L_QREAD                 0x6b    /* 1I / 4O read command     3   1   >=1   */
#define MX25L_WREN                  0x06    /* Write Enable             0   0   0     */ 
#define MX25L_WRDI                  0x04    /* Write Disable            0   0   0     */ 
#define MX25L_RDSR                  0x05    /* Read status register     0   0   >=1   */ 
#define MX25L_RDCR                  0x15    /* Read config register     0   0   >=1   */ 
#define MX25L_WRSR                  0x01    /* Write stat/conf register 0   0   2     */ 
#define MX25L_4PP                   0x38    /* Quad page program        3   0   1-256 */
#define MX25L_SE                    0x20    /* 4Kb Sector erase         3   0   0     */
#define MX25L_BE32                  0x52    /* 32Kb block Erase       3   0   0     */
#define MX25L_BE64                  0xd8    /* 64Kb block Erase       3   0   0     */
#define MX25L_CE                    0xc7    /* Chip erase               0   0   0     */ 
#define MX25L_CE_ALT                0x60    /* Chip erase (alternate)   0   0   0     */ 
#define MX25L_PP                    0x02    /* Page program             3   0   1-256 */
#define MX25L_DP                    0xb9    /* Deep power down          0   0   0     */ 
#define MX25L_RDP                   0xab    /* Release deep power down  0   0   0     */ 
#define MX25L_PGM_SUSPEND           0x75    /* Suspends program         0   0   0     */ 
#define MX25L_ERS_SUSPEND           0xb0    /* Suspends erase           0   0   0     */ 
#define MX25L_PGM_RESUME            0x7A    /* Resume program           0   0   0     */ 
#define MX25L_ERS_RESUME            0x30    /* Resume erase             0   0   0     */ 
#define MX25L_RDID                  0x9f    /* Read identification      0   0   3     */ 
#define MX25L_RES                   0xab    /* Read electronic ID       0   3   1     */
#define MX25L_REMS                  0x90    /* Read manufacture and ID  1   2   >=2   */
#define MX25L_ENSO                  0xb1    /* Enter secured OTP        0   0   0     */ 
#define MX25L_EXSO                  0xc1    /* Exit secured OTP         0   0   0     */ 
#define MX25L_RDSCUR                0x2b    /* Read security register   0   0   0     */ 
#define MX25L_WRSCUR                0x2f    /* Write security register  0   0   0     */ 
#define MX25L_RSTEN                 0x66    /* Reset Enable             0   0   0     */ 
#define MX25L_RST                   0x99    /* Reset Memory             0   0   0     */ 
#define MX25L_RDSFDP                0x5a    /* read out until CS# high  */
#define MX25L_SBL                   0xc0    /* Set Burst Length         */
#define MX25L_SBL_ALT               0x77    /* Set Burst Length         */
#define MX25L_NOP                   0x00    /* No Operation             0   0   0     */ 


/* MX25L Registers ******************************************************************/
/* Read ID (RDID) register values */

#define MX25L_MANUFACTURER          0xc2  /* Macronix manufacturer ID */
#define MX25L3233F_DEVID            0x15  /* MX25L3233F device ID */

/* JEDEC Read ID register values */

#define MX25L_JEDEC_MANUFACTURER         0xc2  /* Macronix manufacturer ID */
#define MX25L_JEDEC_MEMORY_TYPE          0x20  /* MX25Lx  memory type */
#define MX25L_JEDEC_MX25L3233F_CAPACITY  0x16  /* MX25L3233F memory capacity */
#define MX25L_JEDEC_MX25L6433F_CAPACITY  0x17  /* MX25L6433F memory capacity */

/* Status register bit definitions */

#define MX25L_SR_WIP                (1 << 0)  /* Bit 0: Write in progress */
#define MX25L_SR_WEL                (1 << 1)  /* Bit 1: Write enable latch */
#define MX25L_SR_BP_SHIFT           (2)       /* Bits 2-5: Block protect bits */
#define MX25L_SR_BP_MASK            (15 << MX25L_SR_BP_SHIFT)
#define MX25L_SR_QE                 (1 << 6)  /* Bit 6: Quad enable */
#define MX25L_SR_SRWD               (1 << 7)  /* Bit 7: Status register write protect */

/* Configuration registerregister bit definitions */

#define MX25L_CR_ODS                (1 << 0)  /* Bit 0: Output driver strength */
#define MX25L_CR_TB                 (1 << 3)  /* Bit 3: Top/bottom selected */
#define MX25L_CR_DC                 (1 << 6)  /* Bit 6: Dummy cycle */

#define MX25L_DUMMY                 MX25L_NOP
//******************************************************************************
// Private Types
//******************************************************************************

//******************************************************************************
// Private Function Prototypes
//******************************************************************************
//Низкоуровневые функции работы с железом
static void flash_init_hw(Spi_t *Spi);

static void flash_spi_set_nss(void);
static void flash_spi_clr_nss(void);
static void flash_spi_write(uint8_t data);
static uint8_t flash_spi_read(void);
static void flash_spi_write_block(uint8_t *data, uint32_t size);
static void flash_spi_read_block(uint8_t *data, uint32_t size);
//******************************************************************************
// Private Data
//******************************************************************************

static Gpio_t      DiskNss;
static Spi_t       *DiskSpi;

//******************************************************************************
// Public Data
//******************************************************************************
mx25l_dev_t mx25l_dev;
//******************************************************************************
// Private Functions
//******************************************************************************
/*
Инициализация пина выбора микросхемы и подключение SPI
*/
static void flash_init_hw(Spi_t *Spi) 
{
  GpioInit( &DiskNss, FLASH_DISK_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
  DiskSpi = Spi;
}

/*
Установка пина выбора микросхемы
*/
static void flash_spi_set_nss(void) 
{
  GpioWrite( &DiskNss, 0 );
}
/*
Сброс пина выбора микросхемы
*/
static void flash_spi_clr_nss(void) 
{
  GpioWrite( &DiskNss, 1 );
}

static void flash_spi_write(uint8_t data) 
{
  SpiInOut(DiskSpi, data);
}

static uint8_t flash_spi_read(void) 
{
  uint8_t data = SpiInOut( DiskSpi, 0);
  return data;
}

static void flash_spi_write_block(uint8_t *data, uint32_t size) 
{
  for(int i=0; i<size;i++) 
  {
    flash_spi_write(data[i]);
  }
}

static void flash_spi_read_block(uint8_t *data, uint32_t size) 
{
  for(int i=0; i<size;i++) 
  {
    data[i] = flash_spi_read();
  }
}

//******************************************************************************
// Public Functions
//******************************************************************************


/************************************************************************************
 * Name: mx25l_init
 ************************************************************************************/
void mx25l_init(Spi_t *Spi) 
{
  flash_init_hw(Spi);
}

/************************************************************************************
 * Name: mx25l_readid
 ************************************************************************************/
int mx25l_readid(mx25l_dev_t *priv)
{
  uint16_t manufacturer;
  uint16_t memory;
  uint16_t capacity;

  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"priv: %p\r\n", priv);

  /* Lock the SPI bus, configure the bus, and select this FLASH part. */

  //mx25l_lock(priv->dev);
  flash_spi_set_nss();

  /* Send the "Read ID (RDID)" command and read the first three ID bytes */

  flash_spi_write(MX25L_RDID);
  manufacturer = flash_spi_read();
  memory       = flash_spi_read();
  capacity     = flash_spi_read();

  /* Deselect the FLASH and unlock the bus */

  flash_spi_clr_nss();

  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"manufacturer: %02X memory: %02X capacity: %02X\r\n", manufacturer, memory, capacity);

  /* Check for a valid manufacturer and memory type */

  if (manufacturer == MX25L_JEDEC_MANUFACTURER && memory == MX25L_JEDEC_MEMORY_TYPE)
    {
      /* Okay.. is it a FLASH capacity that we understand? */

      if (capacity == MX25L_JEDEC_MX25L3233F_CAPACITY)
        {
           /* Save the FLASH geometry */

           priv->sectorshift = MX25L_MX25L3233F_SECTOR_SHIFT;
           priv->nsectors    = MX25L_MX25L3233F_NSECTORS;
           priv->pageshift   = MX25L_MX25L3233F_PAGE_SHIFT;					
						//syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"MX25L3233F: Sector size = %d, Total sectors = %d\r\n",(1<<priv->sectorshift),priv->nsectors,(1<<priv->pageshift));				
           return 0;
        }
      else if (capacity == MX25L_JEDEC_MX25L6433F_CAPACITY)
        {
           /* Save the FLASH geometry */

           priv->sectorshift = MX25L_MX25L6433F_SECTOR_SHIFT;
           priv->nsectors    = MX25L_MX25L6433F_NSECTORS;
           priv->pageshift   = MX25L_MX25L6433F_PAGE_SHIFT;				
					//syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"MX25L6433F: Sector size = %d, Total sectors = %d\r\n",(1<<priv->sectorshift),priv->nsectors,(1<<priv->pageshift));				
           return 0;
        }
    }

  return -1;
}

/************************************************************************************
 * Name: mx25l_waitwritecomplete
 ************************************************************************************/

void mx25l_waitwritecomplete(mx25l_dev_t *priv)
{
  uint8_t status;

  /* Loop as long as the memory is busy with a write cycle */

  do
    {
      /* Select this FLASH part */

       flash_spi_set_nss();

      /* Send "Read Status Register (RDSR)" command */

      flash_spi_write(MX25L_RDSR);

      /* Send a dummy byte to generate the clock needed to shift out the status */

      status = flash_spi_read();

      /* Deselect the FLASH */

      flash_spi_clr_nss();

      /* Given that writing could take up to few tens of milliseconds, and erasing
       * could take more.  The following short delay in the "busy" case will allow
       * other peripherals to access the SPI bus.
       */

      if ((status & MX25L_SR_WIP) != 0)
        {
         // mx25l_unlock(priv->dev);
          //HAL_Delay(1);
          //mx25l_lock(priv->dev);
        }
    }
  while ((status & MX25L_SR_WIP) != 0);
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Write complete\r\n");
}

/************************************************************************************
 * Name:  mx25l_writeenable
 ************************************************************************************/

void mx25l_writeenable(mx25l_dev_t *priv)
{
  /* Select this FLASH part */

  flash_spi_set_nss();

  /* Send "Write Enable (WREN)" command */

  flash_spi_write(MX25L_WREN);

  /* Deselect the FLASH */

  flash_spi_clr_nss();	
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Write enabled\r\n");
}	

/************************************************************************************
 * Name:  mx25l_writedisable
 ************************************************************************************/

void mx25l_writedisable(mx25l_dev_t *priv)
{
  /* Select this FLASH part */

  flash_spi_set_nss();

  /* Send "Write Disable (WRDI)" command */

  flash_spi_write(MX25L_WRDI);

  /* Deselect the FLASH */

  flash_spi_clr_nss();
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Write disabled\r\n");
}

/************************************************************************************
 * Name:  mx25l_sectorerase (4k)
 ************************************************************************************/

void mx25l_sectorerase(mx25l_dev_t *priv, uint32_t sector)
{
  uint32_t offset;

  offset = sector << priv->sectorshift;
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"sector erase: %08lx\r\n", (long)sector);
  /* Send write enable instruction */

  mx25l_writeenable(priv);

  /* Select this FLASH part */

  flash_spi_set_nss();

  /* Send the "Sector Erase (SE)" or "Block Erase (BE)" instruction
   * that was passed in as the erase type.
   */

  flash_spi_write(MX25L_SE);

  /* Send the sector offset high byte first.  For all of the supported
   * parts, the sector number is completely contained in the first byte
   * and the values used in the following two bytes don't really matter.
   */

  flash_spi_write((offset >> 16) & 0xff);
  flash_spi_write((offset >> 8) & 0xff);
  flash_spi_write(offset & 0xff);

  /* Deselect the FLASH */

  flash_spi_clr_nss();

  mx25l_waitwritecomplete(priv);
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"sector erased\r\n");
}

/************************************************************************************
 * Name:  mx25l_chiperase
 ************************************************************************************/

int mx25l_chiperase(mx25l_dev_t *priv)
{
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"priv: %p\r\n", priv);
  /* Send write enable instruction */

  mx25l_writeenable(priv);

  /* Select this FLASH part */

  flash_spi_set_nss();

  /* Send the "Chip Erase (CE)" instruction */

  flash_spi_write(MX25L_CE);

  /* Deselect the FLASH */

  flash_spi_clr_nss();

  mx25l_waitwritecomplete(priv);
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Chip erased: OK\r\n");	
  return 0;
}

/************************************************************************************
 * Name: mx25l_byteread
 ************************************************************************************/

void mx25l_byteread(mx25l_dev_t *priv, uint8_t *buffer, uint32_t address, uint32_t nbytes)
{
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Read address: %08lx nbytes: %d\r\n", (long)address, (int)nbytes);
  /* Wait for any preceding write or erase operation to complete. */

  mx25l_waitwritecomplete(priv);

  /* Make sure that writing is disabled */

  mx25l_writedisable(priv);

  /* Select this FLASH part */

  flash_spi_set_nss();

  /* Send "Read from Memory " instruction */

  flash_spi_write(MX25L_FAST_READ);

  /* Send the address high byte first. */

  flash_spi_write((address >> 16) & 0xff);
  flash_spi_write((address >> 8) & 0xff);
  flash_spi_write(address & 0xff);

  /* Send a dummy byte */

  flash_spi_write(MX25L_DUMMY);

  /* Then read all of the requested bytes */
  flash_spi_read_block(buffer, nbytes);
  //SPI_RECVBLOCK(priv->dev, buffer, nbytes);

  /* Deselect the FLASH */

  flash_spi_clr_nss();
	//syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Reading\r\n");
}

/************************************************************************************
 * Name:  mx25l_pagewrite
 ************************************************************************************/

void mx25l_pagewrite(mx25l_dev_t *priv, uint8_t *buffer, uint32_t address, uint32_t nbytes)
{	
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Write address: %08lx nwords: %d\r\n", (long)address, (int)nbytes);
      /* Enable the write access to the FLASH */
      
      mx25l_writeenable(priv);
      
      /* Select this FLASH part */

      flash_spi_set_nss();

      /* Send the "Page Program (MX25L_PP)" Command */

      flash_spi_write(MX25L_PP);

      /* Send the address high byte first. */

       flash_spi_write((address >> 16) & 0xff);
       flash_spi_write((address >> 8) & 0xff);
       flash_spi_write(address & 0xff);

      /* Then send the page of data */
      flash_spi_write_block(buffer, nbytes);
      //SPI_SNDBLOCK(priv->dev, buffer, 1 << priv->pageshift);

      /* Deselect the FLASH and setup for the next pass through the loop */

      flash_spi_clr_nss();
      
      /* Wait for any preceding write or erase operation to complete. */

      mx25l_waitwritecomplete(priv);      

      /* Update addresses */

      address += 1 << priv->pageshift;
      buffer  += 1 << priv->pageshift;
  //syslog(LOG_MODULE_DISK,LOG_LEVEL_DEBUG,0,"Written\r\n");
}
/************************************************************************************
 * Name: mx25l_setstop
 ************************************************************************************/
void mx25l_setstop(void) {
  /* Select this FLASH part */

  flash_spi_set_nss();
  /* Send "Deep power down (MX25L_DP)" command */
  flash_spi_write(MX25L_DP);
  /* Deselect the FLASH */
  flash_spi_clr_nss();	
}
/************************************************************************************
 * Name: mx25l_setrun
 ************************************************************************************/
void mx25l_setrun(void) {
  /* Select this FLASH part */
  flash_spi_set_nss();
  /* Send "Release deep power down(MX25L_DP)" command */
  flash_spi_write(MX25L_RDP);
  /* Deselect the FLASH */
  flash_spi_clr_nss();
}

/************************************************************************************
* Name: mx25l_readstatus
************************************************************************************/
uint8_t mx25l_readstatus(mx25l_dev_t *priv)
{
  uint8_t status;
  
  /* Select this FLASH part */
  
  flash_spi_set_nss();
  
  /* Send "Read Status Register (RDSR)" command */
  
  flash_spi_write(MX25L_RDSR);
  
  /* Send a dummy byte to generate the clock needed to shift out the status */
  
  status = flash_spi_read();
  
  /* Deselect the FLASH */
  
  flash_spi_clr_nss();
  
  return status;
}

/************************************************************************************
* Name: mx25l_readconfig
************************************************************************************/
uint8_t mx25l_readconfig(mx25l_dev_t *priv)
{
  uint8_t config;
  
  /* Select this FLASH part */
  
  flash_spi_set_nss();
  
  /* Send "Read Configuration Register (RDCR)" command */
  
  flash_spi_write(MX25L_RDCR);
  
  /* Send a dummy byte to generate the clock needed to shift out the config */
  
  config = flash_spi_read();
  
  /* Deselect the FLASH */
  
  flash_spi_clr_nss();
  
  return config;
}

/************************************************************************************
* Name: mx25l_writestatconf
************************************************************************************/
uint8_t mx25l_writestatconf(mx25l_dev_t *priv, uint8_t status, uint8_t config)
{
  /* Select this FLASH part */
  
  flash_spi_set_nss();
  
  /* Send WRSR command */
  
  flash_spi_write(MX25L_WRSR);
  
  /* Send status */

  flash_spi_write(status);

  /* Send config */

  flash_spi_write(config);
  
  /* Deselect the FLASH */
  
  flash_spi_clr_nss();
  
  return config;
}

/************************************************************************************
* Name: mx25l_chipunlock
************************************************************************************/
int mx25l_chipunlock(mx25l_dev_t *priv)
{
  uint8_t status, config;

  //mx25l_lock();
  do
  {
    mx25l_writeenable(priv);
    status = mx25l_readstatus(priv);
  }
  while ((status & MX25L_SR_WEL) != MX25L_SR_WEL);
  status = 0x00;
  config = 0x00;
  mx25l_writestatconf(priv, status, config);
  mx25l_waitwritecomplete(priv);
  status = mx25l_readstatus(priv);
  //mx25l_unlock();
  if ((status & (MX25L_SR_WEL | MX25L_SR_BP_MASK | MX25L_SR_QE | MX25L_SR_SRWD)) == 0)
  {
    return 0;
  }
  return -1;
}
