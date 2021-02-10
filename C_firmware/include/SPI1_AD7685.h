/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors. 
By using this software you agree to the terms of the associated Analog Devices 
License Agreement.
*********************************************************************************/
/*
 * @file      ADC_channel_read.h
 * @brief     Example to demonstrate ADC driver - modified for ADuCm4050
 * @details
 *            This is the primary include file for the ADC example.
 *
 */

/*=============  I N C L U D E S   =============*/

/* ADC Driver includes */
#include <drivers/spi/adi_spi.h>
#include <drivers/dma/adi_dma.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*=============  D E F I N E S  =============*/

/* Write samples to file */
//#ifndef __CC_ARM
//#define WRITE_SAMPLES_TO_FILE
//#endif

/* SPI Device number */
#define BUFFERSIZE1                256u   
#define SPI_MASTER_DEVICE_NUM1     (1u) 


/*****/
/*=============  PROTOTYPES  =============*/
void ad7685init(void); 
void ad7685_SampleData_Blocking(void);
void ad7685disable(void);
void ad7685Read(unsigned char count, unsigned char *buf);
