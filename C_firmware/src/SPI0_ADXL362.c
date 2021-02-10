/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors. 
By using this software you agree to the terms of the associated Analog Devices 
License Agreement.
*********************************************************************************/
/*!
* @file      SPI0_ADXL362.c
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
#include "SPI0_ADXL362.h"


/*=============  D A T A  =============*/
/* SPI Handle */
ADI_ALIGNED_PRAGMA(4)
ADI_SPI_HANDLE hMDevice ADI_ALIGNED_ATTRIBUTE(4); ;

/* master buffers */
ADI_ALIGNED_PRAGMA(4)
uint8_t masterTx[BUFFERSIZE] ADI_ALIGNED_ATTRIBUTE(4);    
ADI_ALIGNED_PRAGMA(4)
uint8_t masterRx[BUFFERSIZE] ADI_ALIGNED_ATTRIBUTE(4);
/* Memory Required for spi driver */
ADI_ALIGNED_PRAGMA(4)
uint8_t MasterSpidevicemem[ADI_SPI_MEMORY_SIZE] ADI_ALIGNED_ATTRIBUTE(4);
ADI_SPI_TRANSCEIVER Mtransceive;
uint32_t masterHWErrors;
uint8_t Config[16]={
  0x52, //SOFT_RESET, 'R'
  0x00, //THRESH_ACTL
  0x00, //THRESH_ACTH
  0x00, //TIME_ACT
  0x00, //THRESH_INACTL
  0x00, //THRESH_INACTH
  0x00, //TIME_INACTL
  0x00, //TIME_INACTH
  0x30, //ACT_INACT_CTL, Loop Mode
  0x00, //FIFO_CONTROL 
  0x80, //FIFO_SAMPLES
  0x00, //INTMAP1
  0x00, //INTMAP2
  0x97, //FILTER_CTL, 8g, 400Hz
  0x02, //POWER_CTL, Measurement Mode
  0x00, //SELF_TEST
};
uint8_t Buf[16];
extern uint16_t adcData[ADC_DATA_LEN];
/*=============  L O C A L    F U N C T I O N S  =============*/


/*============ DATA=====================*/
//uint16_t check_sampletime = 0; 

/*=============  C O D E  =============*/
void xl362init(void)                        
{ 
    ADI_SPI_RESULT eResult = ADI_SPI_SUCCESS;
    
    /* Open the SPI device. It opens in Master mode by default */
    eResult = adi_spi_Open(SPI_MASTER_DEVICE_NUM, MasterSpidevicemem, ADI_SPI_MEMORY_SIZE, &hMDevice);
    DEBUG_RESULT("Failed to init SPI driver", eResult, ADI_SPI_SUCCESS);
    
    /* Set the bit rate  */
    eResult = adi_spi_SetBitrate(hMDevice, 3250000);                            /* 26MHz / (2*(1+3)) = 3.25MHz */
    DEBUG_RESULT("Failed to set Bitrate", eResult, ADI_SPI_SUCCESS);
    
    /* Set the chip select. */    
    eResult = adi_spi_SetChipSelect(hMDevice, ADI_SPI_CS1);
    DEBUG_RESULT("Failed to set chipselect", eResult, ADI_SPI_SUCCESS);
    
    /* Check PartID */
    do{
        xl362Read(5, XL362_DEVID_AD, &Buf[0]);
        for(uint16_t i=0; i<10000; i++);
    }
    while(Buf[XL362_PARTID]!=0xF2);
    /* Soft Reset */
    xl362Write(1, XL362_SOFT_RESET, &Config[0]);
    for(uint16_t i=0; i<10000; i++);
    /* Setting */
    xl362Write(15, XL362_SOFT_RESET+1, &Config[1]);
    /* Check Setting */
    xl362Read(16, XL362_SOFT_RESET, &Buf[0]);

}

/*
  The fifo read function takes a byte count as an int and a
  pointer to the buffer where to return the data.  When the read
  function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    4) Read each byte
    5) CS_N high
*/
void xl362FifoRead(unsigned int count, unsigned char *buf)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;

    Mtransceive.pTransmitter     = masterTx;
    Mtransceive.TransmitterBytes = 1u;
    Mtransceive.nTxIncrement     = 0u;
    Mtransceive.pReceiver        = masterRx;
    Mtransceive.ReceiverBytes    = count;
    Mtransceive.nRxIncrement     = 1u;
    Mtransceive.bDMA             = true;
    Mtransceive.bRD_CTL          = true;
    
    /* Fill TX buffer */
    masterTx[0u] = XL362_FIFO_READ;
    masterTx[1u] = 0;
    
    eResult = adi_spi_MasterSubmitBuffer(hMDevice, &Mtransceive);
    DEBUG_RESULT("Failed to submit buffer", eResult, ADI_SPI_SUCCESS);
    
    eResult = adi_spi_GetBuffer(hMDevice, &masterHWErrors);
    DEBUG_RESULT("Failed to get buffer", eResult, ADI_SPI_SUCCESS);
    DEBUG_RESULT("HW Error Occured", (uint16_t)masterHWErrors, ADI_SPI_HW_ERROR_NONE);
}

/*
  The read function takes a byte count, a register address and a
  pointer to the buffer where to return the data.  When the read
  function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    3) Send the register address
    4) Read each byte
    5) CS_N high
*/
void xl362Read(unsigned char count, unsigned char regaddr, unsigned char *buf)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;

    Mtransceive.pTransmitter     = masterTx;
    Mtransceive.TransmitterBytes = 2u;
    Mtransceive.nTxIncrement     = 1u;
    Mtransceive.pReceiver        = buf;
    Mtransceive.ReceiverBytes    = count;
    Mtransceive.nRxIncrement     = 1u;
    Mtransceive.bDMA             = true;
    Mtransceive.bRD_CTL          = true;
    
    /* Fill TX buffer */
    masterTx[0u] = XL362_REG_READ;
    masterTx[1u] = regaddr;
    
    eResult = adi_spi_MasterSubmitBuffer(hMDevice, &Mtransceive);
    DEBUG_RESULT("Failed to submit buffer", eResult, ADI_SPI_SUCCESS);
    
    eResult = adi_spi_GetBuffer(hMDevice, &masterHWErrors);
    DEBUG_RESULT("Failed to get buffer", eResult, ADI_SPI_SUCCESS);
    DEBUG_RESULT("HW Error Occured", (uint16_t)masterHWErrors, ADI_SPI_HW_ERROR_NONE);
}  

/*
  The write function takes a byte count, and a pointer to the buffer
  with the data.  The first byte of the data should be the start
  register address, the remaining bytes will be written starting at
  that register.  The mininum bytecount that shoudl be passed is 2,
  one byte of address, followed by a byte of data.  Multiple
  sequential registers can be written with longer byte counts. When
  the write function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    3) Send the register address
    4) Send each byte
    5) CS_N high
*/
void xl362Write(unsigned char count, unsigned char regaddr, unsigned char *buf)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;
    uint32_t i;
    
    Mtransceive.pTransmitter     = masterTx;
    Mtransceive.TransmitterBytes = 2u + count;
    Mtransceive.nTxIncrement     = 1u;
    Mtransceive.pReceiver        = masterRx;
    Mtransceive.ReceiverBytes    = 0u;
    Mtransceive.nRxIncrement     = 1u;
    Mtransceive.bDMA             = true;
    Mtransceive.bRD_CTL          = false;
    
    /* Fill TX buffer */
    masterTx[0u] = XL362_REG_WRITE;
    masterTx[1u] = regaddr;
    for(i=0; i<count; i++){
      masterTx[2u+i] = *buf++;
    }
    
    eResult = adi_spi_MasterSubmitBuffer(hMDevice, &Mtransceive);
    DEBUG_RESULT("Failed to submit buffer", eResult, ADI_SPI_SUCCESS);
    
    eResult = adi_spi_GetBuffer(hMDevice, &masterHWErrors);
    DEBUG_RESULT("Failed to get buffer", eResult, ADI_SPI_SUCCESS);
    DEBUG_RESULT("HW Error Occured", (uint16_t)masterHWErrors, ADI_SPI_HW_ERROR_NONE);
}

void xl362disable(void)
{
    ADI_SPI_RESULT  eResult = ADI_SPI_SUCCESS;
  
    /* Close the SPI */
    eResult = adi_spi_Close(hMDevice);
    DEBUG_RESULT("Failed to close SPI", eResult, ADI_SPI_SUCCESS);
}

void xl362_SampleData_Blocking(uint8_t extra_bits, uint8_t samp_time)
{
    ADI_ADC_RESOLUTION  eResolution;
    int16_t sampletime;
    
    sampletime = (samp_time + 14) * ((uint16_t)pow(2, 2*extra_bits)) * 30 / 65 - 204; //SPI actual read time is needed to modify (204) 
    sampletime = 50; 
    //check_sampletime = sampletime; 
    if (extra_bits>4)
    {
        extra_bits=4;
    }
    
    switch (extra_bits)
    {
    case 0:
        eResolution=ADI_ADC_RESOLUTION_12_BIT;
        break;
    case 1:
        eResolution=ADI_ADC_RESOLUTION_13_BIT;
        break;
    case 2:
        eResolution=ADI_ADC_RESOLUTION_14_BIT;
        break;
    case 3:
        eResolution=ADI_ADC_RESOLUTION_15_BIT;
        break;
    case 4:
        eResolution=ADI_ADC_RESOLUTION_16_BIT;
        break;
    default:
        eResolution=ADI_ADC_RESOLUTION_12_BIT;
    }

//(*((int16_t *)&masterRx[0])) is X-axis data
//(*((int16_t *)&masterRx[2])) is Y-axis data
//(*((int16_t *)&masterRx[4])) is Z-axis data
//(*((int16_t *)&masterRx[6])) is Temperature
    for(uint16_t j=0; j<ADC_NUM_SAMPLES; j++){
        if(sampletime>0){
            for(int16_t i=0; i<sampletime; i++); //3=1us                        //acquisition time
        }
        xl362Read(8, XL362_XDATAL, masterRx);
        switch(eResolution){
        case ADI_ADC_RESOLUTION_12_BIT:  
            adcDataX[j] = (*((int16_t *)&masterRx[0])) + 2048;
            adcDataY[j] = (*((int16_t *)&masterRx[2])) + 2048;
            adcDataZ[j] = (*((int16_t *)&masterRx[4])) + 2048;
            break;
        case ADI_ADC_RESOLUTION_13_BIT:
            adcDataX[j] = ((*((int16_t *)&masterRx[0]))<<1) + 4096;
            adcDataY[j] = ((*((int16_t *)&masterRx[2]))<<1) + 4096;
            adcDataZ[j] = ((*((int16_t *)&masterRx[4]))<<1) + 4096;
            break;
        case ADI_ADC_RESOLUTION_14_BIT:
            adcDataX[j] = ((*((int16_t *)&masterRx[0]))<<2) + 8192;
            adcDataY[j] = ((*((int16_t *)&masterRx[2]))<<2) + 8192;
            adcDataZ[j] = ((*((int16_t *)&masterRx[4]))<<2) + 8192;
            break;
        case ADI_ADC_RESOLUTION_15_BIT:
            adcDataX[j] = ((*((int16_t *)&masterRx[0]))<<3) + 16384;
            adcDataY[j] = ((*((int16_t *)&masterRx[2]))<<3) + 16384;
            adcDataZ[j] = ((*((int16_t *)&masterRx[4]))<<3) + 16384;
            break;
        case ADI_ADC_RESOLUTION_16_BIT:
            adcDataX[j] = ((*((int16_t *)&masterRx[0]))<<4) + 32768;
            adcDataY[j] = ((*((int16_t *)&masterRx[2]))<<4) + 32768;
            adcDataZ[j] = ((*((int16_t *)&masterRx[4]))<<4) + 32768;
            break;
        default:
            adcDataX[j] = (*((int16_t *)&masterRx[0])) + 2048;
            adcDataY[j] = (*((int16_t *)&masterRx[2])) + 2048;
            adcDataZ[j] = (*((int16_t *)&masterRx[4])) + 2048;
            break;
        }    
    }

}

/*****/
