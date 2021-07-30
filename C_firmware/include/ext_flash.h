
#ifndef EXT_FLASH__
#define EXT_FLASH__

//#include "adi_ADuCM4050.h"
#include <adi_spi.h>
#include <adi_dma.h>
#include <stdlib.h>  // For dynamic memory management
#include <drivers/general/adi_drivers_general.h>  // For aligned pragmas
#include <common.h>
#include "ADC_channel_read.h"

// Select which SPI device to use for external flash communication
#define EXT_FLASH_SPI_NUM  2u

#define NUM_FLASH_PAGES_PER_BLOCK  64u
#define NUM_FLASH_BLOCKS           1024u

#define NUM_FLASH_BLOCKS_PER_AXIS  341u   // floor(1024/3)
#define X_AXIS_BLOCK_BOUNDARY      NUM_FLASH_BLOCKS_PER_AXIS
#define Y_AXIS_BLOCK_BOUNDARY      NUM_FLASH_BLOCKS_PER_AXIS*2
#define Z_AXIS_BLOCK_BOUNDARY      NUM_FLASH_BLOCKS_PER_AXIS*3

// Declare typedef to a function that returns an uint16_t 
// and accepts no parameters
typedef uint16_t(*getStatus_t)(void);

extern bool ext_flash_needed;

// Struct for passing to Spi Read and Write functions
// in order to reduce number of passed parameters and 
// ensure easier readability
typedef struct
{
   uint32_t n_bytes_rx;
   uint32_t n_bytes_tx;
   bool bDMA;
   bool blocking;
} SpiRW_s;


// Define Flash Command Values
#define WRITE_ENABLE_FLASH     0x06
#define WRITE_DISABLE_FLASH    0x04
#define PROGRAM_LOAD_FLASH     0x02
#define PROGRAM_EXE_FLASH      0x10
#define GET_FEATURES_FLASH     0x0F
#define SET_FEATURES_FLASH     0x1F
#define PAGE_READ_FLASH        0x13
#define READ_FROM_CACHE_FLASH  0x03
#define BLOCK_ERASE_FLASH      0xD8

// Flash Register Addresses
#define FLASH_LOCK_ADDR        0xA0
#define FLASH_CONFIG_ADDR      0xB0
#define FLASH_STATUS_ADDR      0xC0


#define BITP_FLSH_STAT_OIP 0  // Operation in Progress
#define BITP_FLSH_STAT_WEL 1  // Write Enable

#define BITM_FLSH_STAT_OIP 0x00000001  // Operation in Progress
#define BITM_FLSH_STAT_WEL 0x00000010  // Write Enable

void initExtFlashSPI();
void spiWriteFlash(uint8_t *tx_data, SpiRW_s spi_rw);
void spiReadFlash(uint8_t *rx_buffer, uint8_t *tx_cmd, SpiRW_s spi_rw);

void flashUnlockBlocks(void);
uint8_t flashGetFeatures(uint8_t);
void flashSetFeatures(uint8_t, uint8_t);

// ----- Flash Data Storing ----- //
void flashProgramLoad(uint16_t, uint8_t*, uint32_t);
void flashProgramExecute(uint16_t, uint8_t);
bool isSpi2Busy(void);
void waitForSpi2(void);

void spi2Callback (void *pCBParam, uint32_t nEvent, void *EventArg);
// ---------------------------- //

// ----- Flash Data Saving ----- //
void flashPageRead(uint16_t, uint8_t);
void flashReadFromCache(uint16_t, uint8_t*, uint32_t); 
// ---------------------------- //

void flashEraseBlock(uint16_t, bool);

uint16_t getFlashStatus(void);
uint16_t getFlashConfig(void);
uint16_t getFlashLockState(void);
uint16_t getSpiStatus(void);

void pollBit(getStatus_t, uint32_t, bool); // Generic poll function
bool pollFlashStatus(uint32_t, bool);
bool pollSpiStatus(uint32_t, bool);

void flashWriteEnable(void);
void flashWriteDisable(void);

void prepareFlash(void);

void updatePagePointers(bool, axis_t);
bool checkPagePointers(axis_t);
bool checkAllPagePointers();
void resetPagePointers(axis_t);
uint16_t getBlockAddrRd(axis_t);
uint8_t  getPageAddrRd(axis_t);
uint16_t getBlockAddrWr(axis_t);
uint8_t  getPageAddrWr(axis_t);

#endif  // EXT_FLASH__

