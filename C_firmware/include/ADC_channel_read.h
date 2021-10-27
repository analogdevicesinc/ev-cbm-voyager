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

#ifndef ADC_CHANNEL_READ__
#define ADC_CHANNEL_READ__

/*=============  I N C L U D E S   =============*/

/* ADC Driver includes */
#include <drivers/adc/adi_adc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*=============  D E F I N E S  =============*/
// Defined if using an old 1ax mote in the newer 3-axis system
//#define OLD_MOTE

/* Write samples to file */
//#ifndef __CC_ARM
//#define WRITE_SAMPLES_TO_FILE
//#endif

/* User */
typedef enum 
{
   x_active,
   y_active,
   z_active
} axis_t; //Data handling states



/* ADC Device number */
#define ADC_DEV_NUM         (0u)

/* Sampling period = (SAMPTIME+DLY+16)*ADC_CLK Period*/
/* Total Sample Time Length = ADC_NUM_SAMPLES_RAW*Sampling Period for raw data */ 
#define ADC_PARAM_LEN       2u  //bytes required to hold adc parameter info
#define ADC_PARAM_LEN_S     1u  // Number of sample slots in array that header takes up

/* Declare global variables */
extern uint32_t ADC_NUM_SAMPLES; //bytes required for raw adc data
extern uint32_t ADC_FFT_IDX;     // This is the index where FFT data should start being written to
/* Define total length of 1 adc_buffer, and size of bytes used*/
extern uint16_t ADC_DATA_SIZE;
extern uint32_t ADC_DATA_LEN;


// ADC Data Array Sizing
// 2048B in a page, but we lost a byte due to dummy byte and then needed to go down to 
// next number divisible by 32 for FFT to work
#define FLASH_PAGE_SIZE_B       2048  // Effectively... need to have 2 dummy bytes there at start 
#define FLASH_PAGE_SIZE_S       FLASH_PAGE_SIZE_B/2
// 3B required for the header that's sent to flash for program load but also 
// send a dummy byte of data to column address 0x1 so that ADC data is 16b aligned in flash
#define FLSH_LOAD_HDR_B       4u  
#define FLSH_LOAD_HDR_S       FLSH_LOAD_HDR_B/2   // 1 sample = 2 bytes
#define ADC_ARR_LEN_S         (FLASH_PAGE_SIZE_S + FLSH_LOAD_HDR_S)*2
#define ADC_SAMPLES_PER_BUFF  FLASH_PAGE_SIZE_B/2

// ADC Data Array Boundaries when Acquiring
//8bit:  4 Flsh Cmd | 2048 ADC Data | 4 Flsh Cmd | 2048 ADC Data |
//16bit: 2 Flsh Cmd | 1024 ADC Data | 2 Flsh Cmd | 1024 ADC Data |
#define FLSH_CMD_1ST_END_S     FLSH_LOAD_HDR_S - 1u
#define ADC_DATA_START_1ST_S   FLSH_LOAD_HDR_S
#define ADC_DATA_END_1ST_S     ADC_DATA_START_1ST_S + ADC_SAMPLES_PER_BUFF - 1u
#define FLSH_CMD_2ND_START_S   ADC_DATA_END_1ST_S + 1u
#define FLSH_CMD_2ND_END_S     FLSH_CMD_2ND_START_S + FLSH_LOAD_HDR_S - 1u
#define ADC_DATA_START_2ND_S   FLSH_CMD_2ND_END_S + 1u
#define ADC_DATA_END_2ND_S     ADC_ARR_LEN_S -1u
/////


/*Acquisition time in ADC Clocks*/
#define SAMPTIME    (185u)

/* Delay time between acquisitions*/
#define DLY         (0u)

/*16.16 representation of Vref*/
#define VREF = (uint32_t)(3.3*65536)  
#define VERSION 1.06

/* The below arrays are sized to accomodate the following:
 *    - flash page size = 2048B
 *    - 2048B = 1024 16bit samples
 *    - Create array large enough so that it can hold data for two pages of
 *      flash. Then ADC can fill one half of the array while DMA is moving the other half
 *    - When it comes to TXing the data via radio, 1980B will be read in at a time and 
 *      have FFT performed on it. FFT will be stored contiguously next to raw data
 */
extern uint16_t adcDataX[ADC_ARR_LEN_S];
extern uint16_t adcDataY[ADC_ARR_LEN_S];
extern uint16_t adcDataZ[ADC_ARR_LEN_S];

/*****/
/*=============  PROTOTYPES  =============*/

void ADC_Init(void);
void ADC_SampleData_Blocking_Oversampling(uint8_t, uint8_t);
void ADC_SampleData_Blocking_Averaging(uint8_t, uint16_t);
void ADC_Disable(void);
void ADC_Calc_FFT();
void updateAdcParams(uint32_t, bool);

#endif  // ADC_CHANNEL_READ__
