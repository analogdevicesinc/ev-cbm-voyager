
#include "ext_flash.h"

#if 0
  #define DEBUG_PRINT(a) printf a
#else
  #define DEBUG_PRINT(a) (void)0
#endif
//#define COG true

bool ext_flash_needed   = false;
bool spi2Busy           = false;

// There are 1024 blocks in the flash
// Give each axis 1/3 = 341 blocks each
uint16_t block_addr_wr_x  = 0;
uint8_t  page_addr_wr_x   = 0;
uint16_t block_addr_rd_x  = 0;
uint8_t  page_addr_rd_x   = 0;

uint16_t block_addr_wr_y  = 341;
uint8_t  page_addr_wr_y   = 0;
uint16_t block_addr_rd_y  = 341;
uint8_t  page_addr_rd_y   = 0;

uint16_t block_addr_wr_z  = 682;
uint8_t  page_addr_wr_z   = 0;
uint16_t block_addr_rd_z  = 682;
uint8_t  page_addr_rd_z   = 0;

// Allocate memory for SPI driver
ADI_ALIGNED_PRAGMA(2)
uint8_t spi2_device_mem[ADI_SPI_MEMORY_SIZE] ADI_ALIGNED_ATTRIBUTE(2);

// Data structure used for SPI communications
ADI_SPI_TRANSCEIVER transceive;
ADI_SPI_HANDLE spiDevice;


void initExtFlashSPI()
{
   adi_spi_Open(EXT_FLASH_SPI_NUM, spi2_device_mem, ADI_SPI_MEMORY_SIZE, &spiDevice);

   // Ensure SPI clock is at fastest freq... 
   // NOTE: PCLK going to the peripherals may be divided down already
   pADI_SPI2->DIV = 0;  // Run SPI as fast as possible
   //adi_spi_SetBitrate(spiDevice, 2500000);

   // Flash chip is connected to CS0
   adi_spi_SetChipSelect(spiDevice, ADI_SPI_CS0);

   adi_spi_RegisterCallback(spiDevice, spi2Callback, NULL);

}


void spi2Callback(void *pCBParam, uint32_t nEvent, void *EventArg)
{
   spi2Busy = false;
}

// *tx_data: Array of data to be sent
// n_bytes: number of bytes from array that should be sent
// bDMA: USe DMA for data transfers?
// blocking: Let function complete in the background or not
void spiWriteFlash(uint8_t *tx_data, SpiRW_s spi_rw)
{
   ADI_SPI_RESULT spiResult;

   // Set RxFlush so that all data coming back on MISO is ignored
   // while sending data... only want half-duplex communication
   ADI_SPI_CTL_t SPI_CTL;
   SPI_CTL.VALUE16    =  0;
   SPI_CTL.SPIEN      =  1;  /**< SPI Enable */
   SPI_CTL.MASEN      =  1;  /**< Master Mode Enable */
   SPI_CTL.CPHA       =  0;  /**< Serial Clock Phase Mode */
   SPI_CTL.CPOL       =  0;  /**< Serial Clock Polarity */
   SPI_CTL.WOM        =  0;  /**< SPI Wired-OR Mode */
   SPI_CTL.LSB        =  0;  /**< LSB First Transfer Enable */
   SPI_CTL.TIM        =  0;  /**< SPI Transfer and Interrupt Mode */
   SPI_CTL.ZEN        =  0;  /**< Transmit Zeros Enable */
   SPI_CTL.RXOF       =  0;  /**< Rx Overflow Overwrite Enable */
   SPI_CTL.OEN        =  0;  /**< Slave MISO Output Enable */
   SPI_CTL.LOOPBACK   =  0;  /**< Loopback Enable */
   SPI_CTL.CON        =  1;  /**< Continuous Transfer Enable */ // NOTE: You may want to set this
   SPI_CTL.RFLUSH     =  1;  /**< SPI Rx FIFO Flush Enable */
   SPI_CTL.TFLUSH     =  0;  /**< SPI Tx FIFO Flush Enable */
   SPI_CTL.CSRST      =  0;  /**< Reset Mode for CS Error Bit */
   pADI_SPI2->CTL = SPI_CTL.VALUE16;

   transceive.bDMA    = spi_rw.bDMA;
   transceive.bRD_CTL = false;

   // Receiver
   transceive.pReceiver     = NULL;
   transceive.ReceiverBytes = 0;
   transceive.nRxIncrement  = 0;  // 16bits at a time

   // Transmitter
   transceive.TransmitterBytes = spi_rw.n_bytes_tx;
   transceive.pTransmitter     = tx_data;
   transceive.nTxIncrement     = 1;  // 16bits at a time

   // Ensure that SPI isn't busy w+1ith anything else first
   waitForSpi2();

   //if (spi_rw.blocking) spiResult = adi_spi_MasterReadWrite(spiDevice, &transceive);
   //else                 spiResult = adi_spi_MasterSubmitBuffer(spiDevice, &transceive);

   spi2Busy = true;
   
   spiResult = adi_spi_MasterSubmitBuffer(spiDevice, &transceive);
    
   if (spi_rw.blocking)
   {
        waitForSpi2();
   }

   DEBUG_RESULT("Flash Write Failed", spiResult, ADI_SPI_SUCCESS);   
}




// *rx_buffer: Pointer to receive data structure
// n_bytes_rx: Number of bytes to receive
// bDMA: USe DMA for data transfers?
// blocking: Let function complete in the background or not
// *tx_cmd: Pointer to data structure containing tx command to send
// n_bytes_tx: How many bytes in tx command
void spiReadFlash(uint8_t *rx_buffer, uint8_t *tx_cmd, SpiRW_s spi_rw)
{
   ADI_SPI_RESULT spiResult;

   bool send_tx_cmd = spi_rw.n_bytes_tx > 0u;

   ADI_SPI_CTL_t SPI_CTL;
   SPI_CTL.VALUE16    =  0;
   SPI_CTL.SPIEN      =  1;  /**< SPI Enable */
   SPI_CTL.MASEN      =  1;  /**< Master Mode Enable */
   SPI_CTL.CPHA       =  0;  /**< Serial Clock Phase Mode */
   SPI_CTL.CPOL       =  0;  /**< Serial Clock Polarity */
   SPI_CTL.WOM        =  0;  /**< SPI Wired-OR Mode */
   SPI_CTL.LSB        =  0;  /**< LSB First Transfer Enable */
   SPI_CTL.TIM        =  0;  /**< SPI Transfer and Interrupt Mode */
   SPI_CTL.ZEN        =  1;  /**< Transmit Zeros Enable */
   SPI_CTL.RXOF       =  1;  /**< Rx Overflow Overwrite Enable */
   SPI_CTL.OEN        =  0;  /**< Slave MISO Output Enable */
   SPI_CTL.LOOPBACK   =  0;  /**< Loopback Enable */
   SPI_CTL.CON        =  1;  /**< Continuous Transfer Enable */ // NOTE: You may want to set this
   SPI_CTL.RFLUSH     =  0;  /**< SPI Rx FIFO Flush Enable */
   SPI_CTL.TFLUSH     =  send_tx_cmd;  /**< SPI Tx FIFO Flush Enable */
   SPI_CTL.CSRST      =  0;  /**< Reset Mode for CS Error Bit */
   pADI_SPI2->CTL = SPI_CTL.VALUE16;

   transceive.bDMA    = spi_rw.bDMA;
   transceive.bRD_CTL = send_tx_cmd;

   // Receiver
   transceive.ReceiverBytes = spi_rw.n_bytes_rx;
   transceive.pReceiver     = rx_buffer;
   transceive.nRxIncrement  = 1;  // 16bits at a time

   // Transmitter
   if (send_tx_cmd) {
       // NOTE: if make sure to redfefine tx_cmd in the code that calls this 
       // function. It will get overwritten by SPI data
       transceive.pTransmitter     = tx_cmd;
       transceive.TransmitterBytes = spi_rw.n_bytes_tx;
       transceive.nTxIncrement     = 1;
   }
   else {
       transceive.pTransmitter     = NULL;
       transceive.TransmitterBytes = 0;
       transceive.nTxIncrement     = 0;
   }

   // Ensure that SPI isn't busy with anything else first
   waitForSpi2();

   //if (spi_rw.blocking) spiResult = adi_spi_MasterReadWrite(spiDevice, &transceive);
   //else                 spiResult = adi_spi_MasterSubmitBuffer(spiDevice, &transceive);

   spi2Busy = true;

   spiResult = adi_spi_MasterSubmitBuffer(spiDevice, &transceive);
    
   if (spi_rw.blocking)
   {
        waitForSpi2();
   }
   DEBUG_RESULT("Flash Read Failed", spiResult, ADI_SPI_SUCCESS);   

}


bool isSpi2Busy()
{
   return spi2Busy;
   //bool bMasterComplete= false;
   //adi_spi_isBufferAvailable(spiDevice, &bMasterComplete);
   //return bMasterComplete;
}

void waitForSpi2()
{
    while (isSpi2Busy());
}



// This command needs to be given to the flash before any data can be sent to it
void flashWriteEnable()
{
   SpiRW_s spi_rw;
   uint8_t spi_tx[1];
   spi_tx[0] = WRITE_ENABLE_FLASH;

   spi_rw.n_bytes_rx = 0;
   spi_rw.n_bytes_tx = 1u;
   spi_rw.bDMA       = false;
   spi_rw.blocking   = true;

   spiWriteFlash(spi_tx, spi_rw);

   pollFlashStatus(BITP_FLSH_STAT_WEL, true);
}


// Stop any accidental programming to flash
void flashWriteDisable()
{
   uint8_t spi_tx[1];
   SpiRW_s spi_rw;

   spi_tx[0] = WRITE_DISABLE_FLASH;

   spi_rw.n_bytes_rx = 0;
   spi_rw.n_bytes_tx = 1u;
   spi_rw.bDMA       = false;
   spi_rw.blocking   = true;

   spiWriteFlash(spi_tx, spi_rw);

   pollFlashStatus(BITP_FLSH_STAT_WEL, false);
}

void flashSetFeatures(uint8_t addr, uint8_t data)
{
    SpiRW_s spi_rw;
    uint8_t tx_cmd[3];

    tx_cmd[0] = SET_FEATURES_FLASH;
    tx_cmd[1] = addr;
    tx_cmd[2] = data;

    spi_rw.n_bytes_rx = 0;
    spi_rw.n_bytes_tx = 3u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = true;

    spiWriteFlash(tx_cmd, spi_rw);
}

// Send SET FEATURES command and reg address to flash chip. 
// Followed by a byte of zeros to unlock all blocks
void flashUnlockBlocks()
{
   uint16_t lock_bits;
   lock_bits = flashGetFeatures(FLASH_LOCK_ADDR);
   while (lock_bits != 0X0000)
   {
       flashSetFeatures(FLASH_LOCK_ADDR, 0x0);
#ifndef COG       
       lock_bits = flashGetFeatures(FLASH_LOCK_ADDR);
#else
       lock_bits = 0x0000;
#endif       
       DEBUG_PRINT(("Flash Lock: 0x%x\n", getFlashLockState()));

   }
}


uint8_t flashGetFeatures(uint8_t addr)
{
    SpiRW_s spi_rw;
    uint8_t rd_data;
    uint8_t tx_cmd[2];

    tx_cmd[0] = GET_FEATURES_FLASH;
    tx_cmd[1] = addr;

    spi_rw.n_bytes_rx = 1u;
    spi_rw.n_bytes_tx = 2u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = true;

    spiReadFlash(&rd_data, tx_cmd, spi_rw);

    return (uint16_t)rd_data;
}

// Send GET FEATURES command and reg address to flash chip. 
// Get byte of data back
uint16_t getFlashStatus()
{
   return flashGetFeatures(FLASH_STATUS_ADDR);
}

uint16_t getFlashLockState()
{
   return flashGetFeatures(FLASH_LOCK_ADDR);
}

// Send GET FEATURES command and reg address to flash chip. 
// Get byte of data back
uint16_t getFlashConfig()
{

   return flashGetFeatures(FLASH_CONFIG_ADDR);
}


uint16_t getSpiStatus()
{
   return pADI_SPI2->STAT;
}


// Generic function for polling a bit that accepts
// *getStatus : A pointer to a function that returns the desired status
// poll_bit   : Which bit to poll on
// level      : waiting for bit to go high or low?
void pollBit(getStatus_t getStatus, uint32_t poll_bit, bool level)
{
    uint16_t status;
    do {
       status = (*getStatus)();

       if (level) status = ~status;

       //DEBUG_PRINT(("Wait bit %d to go %d\n", poll_bit, level));

    } while ((status >> poll_bit) & 1);
}


// Poll the flash chip to check if the active operation has finished executing
// poll_bit: Which bit to poll on
// level   : waiting for bit to go high or low?
bool pollFlashStatus(uint32_t poll_bit, bool level)
{
#ifndef COG
   pollBit(&getFlashStatus, poll_bit, level);
#endif
    return true;
}

// Poll the Spi Status Reg to check if the active operation has finished executing
// poll_bit: Which bit to poll on
// level   : waiting for bit to go high or low?
bool pollSpiStatus(uint32_t poll_bit, bool level)
{
   pollBit(&getSpiStatus, poll_bit, level);
   return true;
}


// This function loads the data across the SPI lines
// to the flash memory device. This should be handled in the 
// background by DMA
//
// The header sent to flash is 24 bits:
// [23:16] CMD, [15:12] 0x0, [11:0] column address
void flashProgramLoad(uint16_t column_addr, uint8_t *tx_data, uint32_t n_bytes_data)
{
    SpiRW_s spi_rw;
    uint32_t cmd_and_addr;

    n_bytes_data += 3;  // For Command, Address and Dummy Byte

    // Ensure there is no operation in progress
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

    // Enable Writing
    flashWriteEnable();

    cmd_and_addr = (PROGRAM_LOAD_FLASH << 16) | (column_addr & 0xFFF);
//    DEBUG_PRINT(("Cmd Addr Prog Load: 0x%x\n", cmd_and_addr));

    // Convert this 32 bit header into an array that is 8bit x 4 so SPI driver will use it
    // SPI will start at index 0 and work up... CMD needs to be sent first followed by MSB addr
    tx_data[0] = (cmd_and_addr >> 16) & 0xFF;
    tx_data[1] = (cmd_and_addr >> 8) & 0xFF;
    tx_data[2] = cmd_and_addr & 0xFF;
    // SPI drivers do not allow preloading of TX FIFO even though HW does.
    // Use the start of the data array to hold the cmd and address

    spi_rw.n_bytes_rx = 0;
    spi_rw.n_bytes_tx = n_bytes_data;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = false;

    spiWriteFlash(tx_data, spi_rw);

}


// This function is called as part of the data storage process.
// It is called after the data has been written into flash's cache
// It tells the memory chip load the data from its cache to its NAND flash storage
// The block and page address tell the memory chip where the copy the data
// that is currently in its cache. The header sent to flash is 32 bits:
// [31:24] CMD, [23:16] 0x00, [15:6] block addr, [5:0] page addr
// User provides the addresses and this function will construct the header
void flashProgramExecute(uint16_t block_addr, uint8_t page_addr)
{
    SpiRW_s spi_rw;
    uint32_t cmd_and_addr;
    uint8_t spi_tx_data[4];

    // Make sure no operation is in progress before writing
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

    // Enable Writing
    flashWriteEnable();

    // Make sure the 2 MSBs are 0 becaue page address is only 6 bits
    page_addr = page_addr & 0x3F;

    // Make sure the 6 MSBs are 0 becaue page address is only 10 bits
    block_addr = block_addr & 0x3FF;

    cmd_and_addr = (PROGRAM_EXE_FLASH << 24) | (block_addr << 6) | (page_addr);
    //DEBUG_PRINT(("Cmd Addr Prog Exe: 0x%x\n", cmd_and_addr));

    // Convert this 32 bit header into an array that is 8bit x 4 so SPI driver will use it
    // SPI will start at index 0 and work up... CMD needs to be sent first followed by MSB addr
    spi_tx_data[0] = (cmd_and_addr >> 24) & 0xFF;
    spi_tx_data[1] = (cmd_and_addr >> 16) & 0xFF;
    spi_tx_data[2] = (cmd_and_addr >> 8) & 0xFF;
    spi_tx_data[3] = cmd_and_addr & 0xFF;

    spi_rw.n_bytes_rx = 0;
    spi_rw.n_bytes_tx = 4u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = false;
   
    spiWriteFlash(spi_tx_data, spi_rw);
}


// Load from the specified block/page of flash memory to 
// the memory device's cache
// The header sent to flash is 32 bits:
// [31:24] CMD, [23:16] 0x00, [15:6] block addr, [5:0] page addr
void flashPageRead(uint16_t block_addr, uint8_t page_addr)
{
    SpiRW_s spi_rw;
    uint32_t cmd_and_addr;
    uint8_t spi_tx_data[4];

    flashWriteDisable();

    // Make sure the 2 MSBs are 0 becaue page address is only 6 bits
    page_addr = page_addr & 0x3F;

    // Make sure the 6 MSBs are 0 becaue page address is only 10 bits
    block_addr = block_addr & 0x3FF;

    cmd_and_addr = (PAGE_READ_FLASH << 24) | (block_addr << 6) | (page_addr);
    DEBUG_PRINT(("Cmd Addr Page Read: 0x%x\n", cmd_and_addr));

    // Convert this 32 bit header into an array that is 8bit x 4 so SPI driver will use it
    // SPI will start at index 0 and work up... CMD needs to be sent first followed by MSB addr
    spi_tx_data[0] = (cmd_and_addr >> 24) & 0xFF;
    spi_tx_data[1] = (cmd_and_addr >> 16) & 0xFF;
    spi_tx_data[2] = (cmd_and_addr >> 8) & 0xFF;
    spi_tx_data[3] = cmd_and_addr & 0xFF;

    spi_rw.n_bytes_rx = 0;
    spi_rw.n_bytes_tx = 4u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = true;

    // Make sure no operation is in progress before writing
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

    spiWriteFlash(spi_tx_data, spi_rw);

    // Poll for operation in progress to go low
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);
}


// This function is called as part of the data retrieval process
// It is called after the data has been loaded from a page of flash 
// memory into the memory chip's cache. It tells the memory chip to 
// send the data out from its cache via SPI.
//
// The header sent to flash is 24 bits:
// [23:16] CMD, [15:12] 0x0, [11:0] column address
// The header is then followed by 8 dummy bits before data gets returned
// 
// column_addr: Start address for transaction (cache address)
// rx_buffer: Pointer to array to load data into
// n_bytes_rx: Number of bytes to receive (2048 bytes in a flash page)
void flashReadFromCache(uint16_t column_addr, uint8_t *rx_buffer, uint32_t n_bytes_rx)
{
    SpiRW_s spi_rw;
    uint32_t cmd_and_addr;
    uint8_t spi_tx_data[4];

    cmd_and_addr = (READ_FROM_CACHE_FLASH << 16) | (column_addr & 0xFFF);
    DEBUG_PRINT(("Cmd Addr Cache Read: 0x%x\n", cmd_and_addr));

    // Convert this 32 bit header into an array that is 8bit x 4 so SPI driver will use it
    // SPI will start at index 0 and work up... CMD needs to be sent first followed by MSB addr
    spi_tx_data[0] = (cmd_and_addr >> 16) & 0xFF;
    spi_tx_data[1] = (cmd_and_addr >> 8) & 0xFF;
    spi_tx_data[2] = cmd_and_addr & 0xFF;
    spi_tx_data[3] = 0x00;

    spi_rw.n_bytes_rx = n_bytes_rx;  // 2048 bytes in a flash page
    spi_rw.n_bytes_tx = 4u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = true;
    
    // Make sure no operation is in progress before writing
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

    spiReadFlash(rx_buffer, spi_tx_data, spi_rw);

    // Poll for operation in progress to go low
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

}


// When flash's memory is erased all bits revert to being 1.
// Once a bit is set to 0 it can only be changed back to a 1
// through an erase. Therefore, when a new block is entered we 
// must erase it
void flashEraseBlock(uint16_t block_addr, bool block)
{
    SpiRW_s spi_rw;
    uint32_t cmd_and_addr;
    uint8_t spi_tx_data[4];
    uint8_t page_addr = 0x0;

    // Make sure no operation is in progress before writing
    pollFlashStatus(BITP_FLSH_STAT_OIP, false);

    // Enable Writing
    flashWriteEnable();

    // Make sure the 6 MSBs are 0 becaue page address is only 10 bits
    block_addr = block_addr & 0x3FF;

    cmd_and_addr = (BLOCK_ERASE_FLASH << 24) | (block_addr << 6) | (page_addr);
    DEBUG_PRINT(("Erase: 0x%x\n", cmd_and_addr));

    // Convert this 32 bit header into an array that is 8bit x 4 so SPI driver will use it
    // SPI will start at index 0 and work up... CMD needs to be sent first followed by MSB addr
    spi_tx_data[0] = (cmd_and_addr >> 24) & 0xFF;
    spi_tx_data[1] = (cmd_and_addr >> 16) & 0xFF;
    spi_tx_data[2] = (cmd_and_addr >> 8) & 0xFF;
    spi_tx_data[3] = cmd_and_addr & 0xFF;

    spi_rw.n_bytes_rx = 0;
    spi_rw.n_bytes_tx = 4u;
    spi_rw.bDMA       = false;
    spi_rw.blocking   = block;

    spiWriteFlash(spi_tx_data, spi_rw);

    if (!block)
    {
        // Make sure no operation is in progress before writing
        pollFlashStatus(BITP_FLSH_STAT_OIP, false);
        flashWriteDisable();
    }


}

void prepareFlash(void)
{
    // Unlock the blocks for writing 
    flashUnlockBlocks();

    // Erase the first pages that will be written to
    flashEraseBlock(0x0, true);
    flashEraseBlock(X_AXIS_BLOCK_BOUNDARY, true);
    flashEraseBlock(Y_AXIS_BLOCK_BOUNDARY, true);
    // When a new block is entered by updatePagePointers
    // function it will be erased... just erase each axis'
    // first block here prior to starting

}


// Iterate page and block pointers for specified axis.
// Set pointers back to zero after reaching respective boundaries
void updatePagePointers(bool write, axis_t axis)
{

    uint8_t  *page_addr_wr;
    uint8_t  *page_addr_rd;
    uint16_t *block_addr_wr;
    uint16_t *block_addr_rd;
    uint16_t axis_block_boundary;

    switch (axis) 
    {
        case x_active:
            page_addr_wr        = &page_addr_wr_x;
            page_addr_rd        = &page_addr_rd_x;
            block_addr_wr       = &block_addr_wr_x;
            block_addr_rd       = &block_addr_rd_x;
            axis_block_boundary = X_AXIS_BLOCK_BOUNDARY;
            break;

        case y_active:
            page_addr_wr        = &page_addr_wr_y;
            page_addr_rd        = &page_addr_rd_y;
            block_addr_wr       = &block_addr_wr_y;
            block_addr_rd       = &block_addr_rd_y;
            axis_block_boundary = Y_AXIS_BLOCK_BOUNDARY;
            break;

        case z_active:
            page_addr_wr        = &page_addr_wr_z;
            page_addr_rd        = &page_addr_rd_z;
            block_addr_wr       = &block_addr_wr_z;
            block_addr_rd       = &block_addr_rd_z;
            axis_block_boundary = Z_AXIS_BLOCK_BOUNDARY;
            break;
    }


   if (write) {
      if (*page_addr_wr == NUM_FLASH_PAGES_PER_BLOCK-1) 
      {
         // Entering a new block
         *page_addr_wr = 0;

         if (*block_addr_wr == axis_block_boundary-1)
         {
             *block_addr_wr = 0;
         }
         else
         {
             *block_addr_wr = *block_addr_wr+1;
         }

         // Erase block prior to storing data in it
         flashEraseBlock(*block_addr_wr, false);
      } 
      else 
      {
         *page_addr_wr = *page_addr_wr+1;
      }
      
   } 
   else
   {
      // Page Read
      if (*page_addr_rd == NUM_FLASH_PAGES_PER_BLOCK-1)
      {
          // Entering a new block
          *page_addr_rd = 0;
          if (*block_addr_rd == axis_block_boundary-1)
          {
              *block_addr_rd = 0;
          }
          else
          {
             *block_addr_rd = *block_addr_rd + 1;
          }
      }
      else 
      {
          *page_addr_rd = *page_addr_rd + 1;
      }
   }
}

// Returns true if the read and write pointers are the same
bool checkPagePointers(axis_t axis)
{
    if (axis == x_active)
       return (page_addr_rd_x == page_addr_wr_x) && (block_addr_rd_x == block_addr_wr_x);
    else if (axis == y_active)
       return (page_addr_rd_y == page_addr_wr_y) && (block_addr_rd_y == block_addr_wr_y);
    else if (axis == z_active)
       return (page_addr_rd_z == page_addr_wr_z) && (block_addr_rd_z == block_addr_wr_z);
    else 
       return NULL;
}

bool checkAllPagePointers()
{
    bool x_sent, y_sent, z_sent;
    x_sent = checkPagePointers(x_active);
    y_sent = checkPagePointers(y_active);
    z_sent = checkPagePointers(z_active);
    
    return x_sent && y_sent && z_sent;

}

void resetPagePointers(axis_t axis)
{

   block_addr_wr_x  = X_AXIS_BLOCK_BOUNDARY;
   page_addr_wr_x   = 0;
   block_addr_rd_x  = X_AXIS_BLOCK_BOUNDARY;
   page_addr_rd_x   = 0;
                           
   block_addr_wr_y  = Y_AXIS_BLOCK_BOUNDARY;
   page_addr_wr_y   = 0;
   block_addr_rd_y  = Y_AXIS_BLOCK_BOUNDARY;
   page_addr_rd_y   = 0;
                           
   block_addr_wr_z  = Z_AXIS_BLOCK_BOUNDARY;
   page_addr_wr_z   = 0;
   block_addr_rd_z  = Z_AXIS_BLOCK_BOUNDARY;
   page_addr_rd_z   = 0;

}


uint16_t getBlockAddrRd(axis_t axis)
{
    if (axis == x_active)
       return block_addr_rd_x;
    else if (axis == y_active)
       return block_addr_rd_y;
    else if (axis == z_active)
       return block_addr_rd_z;
    else
      return NULL;
}

uint8_t  getPageAddrRd(axis_t axis)
{
    if (axis == x_active)
       return page_addr_rd_x;
    else if (axis == y_active)
       return page_addr_rd_y;
    else if (axis == z_active)
       return page_addr_rd_z;
    else 
       return NULL;
}

uint16_t getBlockAddrWr(axis_t axis)
{
    if (axis == x_active)
       return block_addr_wr_x;
    else if (axis == y_active)
       return block_addr_wr_y;
    else if (axis == z_active)
       return block_addr_wr_z;
    else 
       return NULL;
}

uint8_t  getPageAddrWr(axis_t axis)
{
    if (axis == x_active)
       return page_addr_wr_x;
    else if (axis == y_active)
       return page_addr_wr_y;
    else if (axis == z_active)
       return page_addr_wr_z;
    else 
       return NULL;
}

