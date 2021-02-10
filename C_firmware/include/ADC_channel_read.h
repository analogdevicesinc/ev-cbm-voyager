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

/* ADC Device number */
#define ADC_DEV_NUM         (0u)

/* Sampling period = (SAMPTIME+DLY+16)*ADC_CLK Period*/
/* Total Sample Time Length = ADC_NUM_SAMPLES_RAW*Sampling Period for raw data */ 
#define ADC_PARAM_LEN       2  //bytes required to hold adc parameter info
#define ADC_NUM_SAMPLES     512 //bytes required for raw adc data
#define ADC_FFT_IDX         ADC_NUM_SAMPLES + ADC_PARAM_LEN //index of fft values
#define ADC_FFT_SCALER      ((float) (1.0/ADC_NUM_SAMPLES)) 

/* Define total length of 1 adc_buffer, and size of bytes used*/
#define ADC_DATA_LEN        (ADC_NUM_SAMPLES + (ADC_NUM_SAMPLES >> 1) + ADC_PARAM_LEN)
#define ADC_DATA_SIZE       (sizeof(uint16_t)*ADC_DATA_LEN)

/*Acquisition time in ADC Clocks*/
#define SAMPTIME    (185u)

/* Delay time between acquisitions*/
#define DLY         (0u)

/*16.16 representation of Vref*/
#define VREF = (uint32_t)(3.3*65536)  
#define VERSION 1.04

extern uint16_t adcDataX[ADC_DATA_LEN];
extern uint16_t adcDataY[ADC_DATA_LEN];
extern uint16_t adcDataZ[ADC_DATA_LEN];

/*****/
/*=============  PROTOTYPES  =============*/

void ADC_Init(void);
void ADC_SampleData_Blocking_Oversampling(uint8_t, uint8_t);
void ADC_SampleData_Blocking_Averaging(uint8_t, uint16_t);
void ADC_Disable(void);
void ADC_Calc_FFT(uint16_t);
