/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors. 
By using this software you agree to the terms of the associated Analog Devices 
License Agreement.
*********************************************************************************/

/*!
* @file      ADC_channel_read.c
* @brief     Example to demonstrate ADC Driver - modified for ADuCM4050
*
* @details
*            This is the primary source file for the ADC example, showing how
*            to read an analog input channel.
*
*/

/*=============  I N C L U D E S   =============*/

/* Managed drivers and/or services include */
#include <drivers/adc/adi_adc.h>
#include <drivers/pwr/adi_pwr.h>
#include <drivers/general/adi_drivers_general.h>
#include <common.h>
#include <assert.h>
#include <stdio.h>
#include <arm_math.h>

/* ADC example include */
#include "ADC_channel_read.h"

/* FFT operation selects */ 
#define FFT_FORWARD_TRANSFORM   0
#define FFT_NORMAL_ORDER_OUTPUT 1

/*=============  D A T A  =============*/

/* ADC Handle */
ADI_ALIGNED_PRAGMA(4)
ADI_ADC_HANDLE hDevice;

/* Memory Required for adc driver */
ADI_ALIGNED_PRAGMA(4)
static uint8_t DeviceMemory[ADI_ADC_MEMORY_SIZE];

/* Allocating space for ADC raw and fft buffers */
ADI_ALIGNED_PRAGMA(4)
uint16_t adcDataX[ADC_DATA_LEN];
uint16_t adcDataY[ADC_DATA_LEN];
uint16_t adcDataZ[ADC_DATA_LEN];

arm_rfft_fast_instance_f32 fftInst;
float fftInBuf[ADC_NUM_SAMPLES];
float fftOutBuf[ADC_NUM_SAMPLES << 1];
float fftMagOutBuf[ADC_NUM_SAMPLES >> 1];

/*Battery Voltage*/
uint32_t *pVbat;

/*====================  L O C A L    F U N C T I O N S  ======================*/

static void usleep(uint32_t usec);

/*===================== D A T A ==============================================*/

extern uint32_t aclock_freq;

/*===================== C O D E  =============================================*/

void ADC_Init(void)
{
    ADI_ADC_RESULT  eResult = ADI_ADC_SUCCESS;  
    bool bCalibrationDone = false;
    
    /* Open the ADC device */
    eResult = adi_adc_Open(ADC_DEV_NUM, DeviceMemory, sizeof(DeviceMemory), &hDevice);
    DEBUG_RESULT("Failed to open ADC device",eResult, ADI_ADC_SUCCESS);

    /* Power up ADC */
    eResult = adi_adc_PowerUp (hDevice, true);
    DEBUG_RESULT("Failed to power up ADC", eResult, ADI_ADC_SUCCESS);

    /* Set ADC reference */
    eResult = adi_adc_SetVrefSource (hDevice, ADI_ADC_VREF_SRC_EXT);
    DEBUG_RESULT("Failed to set ADC reference", eResult, ADI_ADC_SUCCESS);

    /* Enable ADC sub system */
    eResult = adi_adc_EnableADCSubSystem (hDevice, true);
    DEBUG_RESULT("Failed to enable ADC sub system", eResult, ADI_ADC_SUCCESS);

    /* Wait for 5.0ms */
     usleep (5000);

    /* Start calibration */
    eResult = adi_adc_StartCalibration (hDevice);
    DEBUG_RESULT("Failed to start calibration", eResult, ADI_ADC_SUCCESS);

    /* Wait until calibration is done */
    while (!bCalibrationDone)
    {
        eResult = adi_adc_IsCalibrationDone (hDevice, &bCalibrationDone);
        DEBUG_RESULT("Failed to get the calibration status", eResult, ADI_ADC_SUCCESS);
    }

    if (arm_rfft_fast_init_f32(&fftInst, ADC_NUM_SAMPLES) != ARM_MATH_SUCCESS)
        while (1);
}


void ADC_SampleData_Blocking_Oversampling(uint8_t extra_bits, uint8_t samp_time)
{
    ADI_ADC_RESULT  eResult = ADI_ADC_SUCCESS;
    ADI_ADC_BUFFER Buffer;
    ADI_ADC_RESOLUTION eResolution;
    
    /* Set the delay time */
    eResult = adi_adc_SetDelayTime ( hDevice, DLY);
    DEBUG_RESULT("Failed to set the Delay time ", eResult, ADI_ADC_SUCCESS);

    /* Set the acquisition time. (Application need to change it based on the impedence) */
    eResult = adi_adc_SetAcquisitionTime ( hDevice, samp_time);
    DEBUG_RESULT("Failed to set the acquisition time ", eResult, ADI_ADC_SUCCESS);

    /* Populate the buffer structure */
    Buffer.nBuffSize = sizeof(uint16_t)*ADC_DATA_LEN;
    Buffer.nChannels = ADI_ADC_CHANNEL_0;
    Buffer.nNumConversionPasses = ADC_DATA_LEN;
    Buffer.pDataBuffer = adcDataX; // Send X-axis data first 
    
    eResolution=ADI_ADC_RESOLUTION_12_BIT; // set resolution to always be 12-bit, no oversample
    
    /* Enable oversampling */
    eResult = adi_adc_SetResolution (hDevice, eResolution);
    DEBUG_RESULT("Failed to set resolution", eResult, ADI_ADC_SUCCESS);
    
    /* Submit the buffer to the driver */
    eResult = adi_adc_SubmitBuffer (hDevice, &Buffer);
    DEBUG_RESULT("Failed to submit buffer ", eResult, ADI_ADC_SUCCESS);

    /* Enable the ADC */
    eResult = adi_adc_Enable (hDevice, true);
    DEBUG_RESULT("Failed to enable the ADC for sampling ", eResult, ADI_ADC_SUCCESS);

    ADI_ADC_BUFFER* pAdcBuffer;
    eResult = adi_adc_GetBuffer (hDevice, &pAdcBuffer);
    DEBUG_RESULT("Failed to Get Buffer ", eResult, ADI_ADC_SUCCESS);

}


void ADC_Calc_FFT(uint16_t adc_num_samples)
{
    //X_AXIS      
    for (int i = 0; i < ADC_NUM_SAMPLES; i++)
        fftInBuf[i] = (float)adcDataX[i + ADC_PARAM_LEN];

    arm_rfft_fast_f32(&fftInst, fftInBuf, fftOutBuf, 0);
    arm_cmplx_mag_f32(fftOutBuf, fftMagOutBuf, ADC_NUM_SAMPLES >> 1);

    for (int i = 0; i < ADC_NUM_SAMPLES >> 1; i++){
        adcDataX[ADC_FFT_IDX + i] = (uint16_t) (ADC_FFT_SCALER*fftMagOutBuf[i]);
        
//        Optionally fill other arrays with dummy data (for testing old motes)
#ifdef OLD_MOTE
        adcDataY[ADC_FFT_IDX + i] = (uint16_t) (ADC_FFT_SCALER*fftMagOutBuf[i]);
        adcDataZ[ADC_FFT_IDX + i] = (uint16_t) (ADC_FFT_SCALER*fftMagOutBuf[i]);
#endif
    }
    
#ifndef OLD_MOTE
    //Y_AXIS
    for (int i = 0; i < ADC_NUM_SAMPLES; i++)
        fftInBuf[i] = adcDataY[i + ADC_PARAM_LEN];

    arm_rfft_fast_f32(&fftInst, fftInBuf, fftOutBuf, 0);
    arm_cmplx_mag_f32(fftOutBuf, fftMagOutBuf, ADC_NUM_SAMPLES >> 1);

    for (int i = 0; i < ADC_NUM_SAMPLES >> 1; i++)
        adcDataY[ADC_FFT_IDX + i] = (uint16_t) (ADC_FFT_SCALER*fftMagOutBuf[i]);

    //Z_AXIS
    for (int i = 0; i < ADC_NUM_SAMPLES; i++)
        fftInBuf[i] = adcDataZ[i + ADC_PARAM_LEN];

    arm_rfft_fast_f32(&fftInst, fftInBuf, fftOutBuf, 0);
    arm_cmplx_mag_f32(fftOutBuf, fftMagOutBuf, ADC_NUM_SAMPLES >> 1);

    for (int i = 0; i < ADC_NUM_SAMPLES >> 1; i++)
        adcDataZ[ADC_FFT_IDX + i] = (uint16_t) (ADC_FFT_SCALER*fftMagOutBuf[i]);
#else
    //copy x to y and z buffs
    for (int i = ADC_PARAM_LEN; i < ADC_FFT_IDX; i++){
        adcDataY[i] = adcDataX[i];
        adcDataZ[i] = adcDataX[i];
    }
#endif
}


void ADC_Disable(void)
{
    ADI_ADC_RESULT  eResult = ADI_ADC_SUCCESS;
  
    /* Disable the ADC */
    eResult = adi_adc_Enable (hDevice, false);
    DEBUG_RESULT("Failed to disable ADC for sampling ", eResult, ADI_ADC_SUCCESS);

    /* Close the ADC */
    eResult = adi_adc_Close (hDevice);
    DEBUG_RESULT("Failed to close ADC", eResult, ADI_ADC_SUCCESS);
}


/* Approximately wait for minimum 1 usec */
static void usleep(uint32_t usec)
{
    volatile int y = 0;
    while (y++ < usec) {
        volatile int x = 0;
        while (x < 16) {
        x++;
        }
    }
}