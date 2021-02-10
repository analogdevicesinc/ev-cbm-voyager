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
extern uint16_t adcData[ADC_DATA_LEN];

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

    /*Enable CONV, P2_11*/
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
  uint16_t j;
  j = ADC_PARAM_LEN; 
  while(j < ADC_FFT_IDX) {
    
  // Check to see if ad7685 set
    if(check_ad7685 == true) { //Sampling scheduler reached max val
      check_ad7685 = false; 
      // If set, read data to x,y,z buffers, shift data to 16 bit
      ad7685Read(6, masterRx1);
      adcDataX[j] = (((uint16_t)masterRx1[0])<<8) | masterRx1[1];  //X-axis data
      adcDataY[j] = (((uint16_t)masterRx1[2])<<8) | masterRx1[3];  //Y-axis data
      adcDataZ[j] = (((uint16_t)masterRx1[4])<<8) | masterRx1[5];  //Z-axis data
      j++; 
    }
  }
}
