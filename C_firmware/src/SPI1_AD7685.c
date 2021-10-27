/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated Analog Devices
License Agreement.
*********************************************************************************/
/*!
* @file      SPI1_AD7685.c
* @brief     Example to demonstrate ADC Driver - modified for ADuCM4050
*
* @details
*            This is the primary source file for the ADC example, showing how
*            to read an analog input channel.
*
*/

/*=============  I N C L U D E S   =============*/

/* Managed drivers and/or services include */
#include <drivers/pwr/adi_pwr.h>
#include <drivers/gpio/adi_gpio.h>
#include <drivers/spi/adi_spi.h>         /* 02/27/2019  Fan added */
#include <drivers/dma/adi_dma.h>         /* 02/27/2019  Fan added */
#include <drivers/general/adi_drivers_general.h>
#include <common.h>
#include <assert.h>
#include <stdio.h>
#include <arm_math.h>

/* ADC example include */
#include "ADC_channel_read.h"
#include "scheduler.h"
#include "SmartMesh_RF_cog.h"
#include "SPI1_AD7685.h"
#include "ext_flash.h"

#if 0
  #define DEBUG_PRINT(a) printf a
#else
  #define DEBUG_PRINT(a) (void)0
#endif

/*=============  D A T A  =============*/
/* SPI Handle */
ADI_ALIGNED_PRAGMA(4)
ADI_SPI_HANDLE hMDevice1 ADI_ALIGNED_ATTRIBUTE(4); ;

/* master buffers */
ADI_ALIGNED_PRAGMA(4)
uint8_t masterTx1[BUFFERSIZE1] ADI_ALIGNED_ATTRIBUTE(4);
ADI_ALIGNED_PRAGMA(4)
uint8_t masterRx1[BUFFERSIZE1] ADI_ALIGNED_ATTRIBUTE(4);
/* Memory Required for spi driver */
ADI_ALIGNED_PRAGMA(4)
uint8_t MasterSpidevicemem1[ADI_SPI_MEMORY_SIZE] ADI_ALIGNED_ATTRIBUTE(4);
ADI_SPI_TRANSCEIVER Mtransceive1;

uint32_t masterHWErrors1;

/*===================  L O C A L    F U N C T I O N S  =======================*/

/*===================  C O D E  ==============================================*/
void ad7685init(void)
{
    ADI_SPI_RESULT eResult = ADI_SPI_SUCCESS;

    /* Open the SPI device. It opens in Master mode by default */
    eResult = adi_spi_Open(SPI_MASTER_DEVICE_NUM1, MasterSpidevicemem1, ADI_SPI_MEMORY_SIZE, &hMDevice1);
    DEBUG_RESULT("Failed to init SPI driver", eResult, ADI_SPI_SUCCESS);

    /* Set the bit rate */
    eResult = adi_spi_SetBitrate(hMDevice1, 3250000);                            /* 26MHz / (2*(1+3)) = 3.25MHz */
    DEBUG_RESULT("Failed to set Bitrate", eResult, ADI_SPI_SUCCESS);

    /* Set the chip select */
    eResult = adi_spi_SetChipSelect(hMDevice1, ADI_SPI_CS_NONE);
    DEBUG_RESULT("Failed to set chipselect", eResult, ADI_SPI_SUCCESS);

    /* Set the clock phase */
    eResult = adi_spi_SetClockPhase(hMDevice1, true);
    DEBUG_RESULT("Failed to set ClockPhase", eResult, ADI_SPI_SUCCESS);

}

void ad7685Read(unsigned char count, unsigned char *buf)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;

    /*Enable CONV, P2_11 ... This will begin ADC conversions*/
    adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_1);

    /*Acquisition*/
    Mtransceive1.pTransmitter     = masterTx1;
    Mtransceive1.TransmitterBytes = 0;
    Mtransceive1.nTxIncrement     = 0u;
    Mtransceive1.pReceiver        = buf;
    Mtransceive1.ReceiverBytes    = count;
    Mtransceive1.nRxIncrement     = 1u;
    Mtransceive1.bDMA             = true;
    Mtransceive1.bRD_CTL          = false;

    /* Fill TX buffer */
    masterTx1[0u] = 0;
    masterTx1[1u] = 0;

    eResult = adi_spi_MasterSubmitBuffer(hMDevice1, &Mtransceive1);
    DEBUG_RESULT("Failed to submit buffer", eResult, ADI_SPI_SUCCESS);

    /*Enable CONV, P2_11*/
    //adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_1);

    eResult = adi_spi_GetBuffer(hMDevice1, &masterHWErrors1);
    DEBUG_RESULT("Failed to get buffer", eResult, ADI_SPI_SUCCESS);
    DEBUG_RESULT("HW Error Occured", (uint16_t)masterHWErrors1, ADI_SPI_HW_ERROR_NONE);


    /*Disable CONV, P2_11*/
    adi_gpio_SetLow(ADI_GPIO_PORT2, ADI_GPIO_PIN_1);
}

void ad7685disable(void)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;

    /* Close the SPI */
    eResult = adi_spi_Close(hMDevice1);
    DEBUG_RESULT("Failed to close SPI", eResult, ADI_SPI_SUCCESS);
}

void ad7685_SampleData_Blocking(void)
{
  // Collect the defined number of samples, at the sample rate defined by the FFT scheduler
  uint16_t i = 0, j = 0;
  uint32_t numSamples;
  uint8_t  axis_info;
  uint16_t block_addr;
  uint8_t  page_addr;
  bool x_en      = 0, y_en      = 0, z_en      = 0;
  bool load_x    = 0, load_y    = 0, load_z    = 0;
  bool loading_x = 0, loading_y = 0, loading_z = 0;
  bool active_wr_buff_x = 0;
  bool active_wr_buff_y = 0;
  bool active_wr_buff_z = 0;

  uint8_t *wr_ptr;  // Pointer to a byte


  axis_info = getAxisInfo();
  if (axis_info == XYZ || axis_info == XY || axis_info == XZ || axis_info == X) 
      x_en = 1;

  if  (axis_info == XYZ || axis_info == XY || axis_info == YZ || axis_info == Y)
      y_en = 1;

  if  (axis_info == XYZ || axis_info == XZ || axis_info == YZ || axis_info == Z)
      z_en = 1;

  DEBUG_PRINT(("x%d, y%d, z%d\n", x_en, y_en, z_en));

  numSamples = getAdcNumSamples(); 
  j          = ADC_DATA_START_1ST_S;

  while(i < numSamples || (load_x || loading_x || load_y || loading_y || load_z || loading_z))
  { 
    // Check to see if ad7685 set
    if(check_ad7685 == true && i < numSamples) 
    {
      check_ad7685 = false;
      // If set, read data to x,y,z buffers, shift data to 16 bit
      ad7685Read(6, masterRx1);
      adcDataX[j] = (((uint16_t)masterRx1[0])<<8) | masterRx1[1];  //X-axis data
      adcDataY[j] = (((uint16_t)masterRx1[2])<<8) | masterRx1[3];  //Y-axis data
      adcDataZ[j] = (((uint16_t)masterRx1[4])<<8) | masterRx1[5];  //Z-axis data
      
      i++;

      // Control write pointer, j... And begin SPI transactions to flash if required
      // Due to limitations in SPI driver TX fifo cannot be preloaded with flash command
      // so it needs to be stored in the same array as the data to allow the required 
      // continuous transfers... Jump over those positions in the array when they are reached
      if (j == ADC_DATA_END_1ST_S)
      {
          j = ADC_DATA_START_2ND_S;
          load_x = x_en;
          load_y = y_en;
          load_z = z_en;
      }
      else if (j == ADC_DATA_END_2ND_S)
      {
          j = ADC_DATA_START_1ST_S;
          load_x = x_en;
          load_y = y_en;
          load_z = z_en;
      }
      else
      {
          j++;
      }

      if ((i == numSamples - 1) && ext_flash_needed)
      {
          load_x = x_en;
          load_y = y_en;
          load_z = z_en;
      }
    } 

    if (load_x)
    {
        load_x    = false;
        loading_x = true;
        if (!active_wr_buff_x)  // Ping
        {
            wr_ptr = (uint8_t*)&adcDataX[0];
            wr_ptr++;  // First byte of array is reserved/unused when acquiring
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        else  //Pong
        {   
            wr_ptr = (uint8_t*)&adcDataX[FLSH_CMD_2ND_START_S];
            wr_ptr++;
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        active_wr_buff_x = !active_wr_buff_x;
    }
    else if (loading_x)
    {   
        if (!isSpi2Busy())  // Check if program load has completed
        {  
            loading_x = false;
            DEBUG_PRINT(("X Ex\n"));
            block_addr = getBlockAddrWr(x_active);
            page_addr  = getPageAddrWr(x_active);
            flashProgramExecute(block_addr, page_addr);
            updatePagePointers(true, x_active);
        }
    }
    else if (load_y)
    {
        load_y    = false;
        loading_y = true;
        if (!active_wr_buff_y)
        {
            wr_ptr = (uint8_t*)&adcDataY[0];
            wr_ptr++;  // First byte of array is reserved/unused when acquiring
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        else  //Pong
        {
            wr_ptr = (uint8_t*)&adcDataY[FLSH_CMD_2ND_START_S];
            wr_ptr++;
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        active_wr_buff_y = !active_wr_buff_y;
    }
    else if (loading_y)
    {   
        if (!isSpi2Busy())
        {  
            loading_y = false;
            DEBUG_PRINT(("Y Ex\n"));
            block_addr = getBlockAddrWr(y_active);
            page_addr  = getPageAddrWr(y_active);
            flashProgramExecute(block_addr, page_addr);
            updatePagePointers(true, y_active);
        }
    }
    else if (load_z)
    {
        load_z    = false;
        loading_z = true;
        if (!active_wr_buff_z) // Ping
        {
            wr_ptr = (uint8_t*)&adcDataZ[0];
            wr_ptr++;  // First byte of array is reserved/unused when acquiring
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        else  //Pong
        {
            wr_ptr = (uint8_t*)&adcDataZ[FLSH_CMD_2ND_START_S];
            wr_ptr++;
            flashProgramLoad(0x0, wr_ptr, FLASH_PAGE_SIZE_B);
        }
        active_wr_buff_z = !active_wr_buff_z;
    }
    else if (loading_z)
    {   
        if (!isSpi2Busy())
        {  
            loading_z  = false;
            DEBUG_PRINT(("Z Ex\n"));
            block_addr = getBlockAddrWr(z_active);
            page_addr  = getPageAddrWr(z_active);
            flashProgramExecute(block_addr, page_addr);
            updatePagePointers(true, z_active);
        }
    }
  }
}
