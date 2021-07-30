/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated Analog Devices
License Agreement.
*********************************************************************************/

/*!
* @file      scheduler.c
* @brief     Code for running scheduler
*
* @details
*            Scheduler can be selected as RTC or GP timer
*
*/
/*=======================  I N C L U D E S   =================================*/

#include <common.h>

#include "SPI1_AD7685.h"
#include "scheduler.h"
#include "SmartMesh_RF_cog.h"
#include <adi_rtc.h>

// For printf statements
#include "stdio.h"
#include "stdint.h"

bool timerTicked = false;

/*=============  D A T A  =============*/

ADI_TMR_EVENT_CONFIG     evtConfig;
ADI_PWR_RESULT           ePwrResult;
ADI_TMR_CONFIG           tmrConfig;
ADI_TMR_CONFIG           samplingTmrConfig;
ADI_TMR_RESULT           eResult;
uint32_t                 nTimeout;

volatile static uint32_t gNumGp0Timeouts = 0u;
bool check_ad7685 = false; //Controls FFT readings

/* Clocks */
extern uint16_t          HDIV;
extern uint16_t          PDIV;
extern uint32_t          hfosc_freq;
extern uint32_t          pclk_freq;
extern uint32_t          lfosc_freq;
extern uint32_t          lfxtal_freq;

/* Flags */
extern bool              idle;
extern int               waitDone;
extern int               timerTicks;
extern int               FFT_Samples_Timeout;

/* Application variables struct for SmartMesh */
extern app_vars_t        app_vars;

extern uint32_t lastRtcCount;
extern ADI_RTC_HANDLE hDevice0;


/*==========================  C O D E  =======================================*/

void StartScheduler(scheduler_t scheduler)
{
    ADI_TMR_CLOCK_SOURCE source;

    DisableGPTimer();   //Make sure timer is disabled before enabling
    if (scheduler.tick_ms>500)
    {
        source=ADI_TMR_CLOCK_LFOSC;
    }
    else
    {
        source=ADI_TMR_CLOCK_HFOSC;
    }

    if (SetupGPTimer(scheduler.tick_ms, source)<0)
    {
        DEBUG_MESSAGE("Unsuccessful setup of GP Timer");
    }
    else
    {
        EnableGPTimer();
    }


}


void StartSamplingScheduler(scheduler_t_us scheduler)
{
    ADI_TMR_CLOCK_SOURCE source;

    DisableSamplingTimer();   //Make sure timer is disabled before enabled, otherwise driver has a problem.
    source=ADI_TMR_CLOCK_HFOSC;

    if (SetupSamplingTimer(scheduler.tick_us, source)<0)
    {
        DEBUG_MESSAGE("Unsuccessful setup of Sampling Timer");
    }
    else
    {
        EnableSamplingTimer();
    }
}

int8_t SetupGPTimer(uint32_t tick_in_ms,ADI_TMR_CLOCK_SOURCE tmr_clk)
{

    uint32_t  clock_freq;
    float     temp1, temp2, temp3, temp4;
    uint16_t  load_value;

    /* Set up GP0 callback function */
    eResult = adi_tmr_Init(ADI_TMR_DEVICE_GP0, GP0CallbackFunction, NULL, true);

    switch(tmr_clk)
    {
        case ADI_TMR_CLOCK_PCLK:
            clock_freq=pclk_freq;
            break;
        case ADI_TMR_CLOCK_HFOSC:
            clock_freq=hfosc_freq;
            break;
        case ADI_TMR_CLOCK_LFOSC:
            clock_freq=lfosc_freq;
            break;
        case ADI_TMR_CLOCK_LFXTAL:
            clock_freq=lfxtal_freq;
            break;
        default:
            clock_freq=pclk_freq;
            break;
    }

    //Calculate required LOAD based on 256 prescaler
    temp1=(float)tick_in_ms;
    temp2=(float)clock_freq;
    temp3=temp1*temp2;
    temp4=temp3/256000.0;
    load_value=(uint16_t)(temp4);

    /* Configure GP0 to have the desired period */
    tmrConfig.bCountingUp  = false;
    tmrConfig.bPeriodic    = true;
    /* Prescaler is fixed to 256 for now. This means that the counter in the
    timer will increment once every 256 clock cycles */
    tmrConfig.ePrescaler   = ADI_TMR_PRESCALER_256;
    tmrConfig.eClockSource = tmr_clk;
    tmrConfig.nLoad        = load_value;
    tmrConfig.nAsyncLoad   = load_value;
    tmrConfig.bReloading   = false;
    tmrConfig.bSyncBypass  = false;
    eResult = adi_tmr_ConfigTimer(ADI_TMR_DEVICE_GP0, &tmrConfig);
    if (eResult!=ADI_TMR_SUCCESS)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


int8_t SetupSamplingTimer(uint32_t tick_in_us,ADI_TMR_CLOCK_SOURCE tmr_clk)
{
    // Counts out acquisition time period, triggers interrupt
    // Seperate callback function written to handle interrupt
    // Callback should read from adc ad7685Read() and set a data_rdy flag
    uint32_t  clock_freq;
    float     temp1, temp2, temp3, temp4;
    uint16_t  load_value;

    /* Set up GP1 callback function */
    eResult = adi_tmr_Init(ADI_TMR_DEVICE_GP1, SamplingTimeCallbackFunction, NULL, true);

    switch(tmr_clk)
    {
        case ADI_TMR_CLOCK_PCLK:
            clock_freq=pclk_freq;
            break;
        case ADI_TMR_CLOCK_HFOSC:
            clock_freq=hfosc_freq;
            break;
        case ADI_TMR_CLOCK_LFOSC:
            clock_freq=lfosc_freq;
            break;
        case ADI_TMR_CLOCK_LFXTAL:
            clock_freq=lfxtal_freq;
            break;
        default:
            clock_freq=pclk_freq;
            break;
    }

    //Calculate required LOAD based on 16 prescaler
    temp1=(float)tick_in_us;
    temp2=(float)clock_freq;
    temp3=temp1*temp2;
    temp4=temp3/16000000.0;
    load_value=(uint16_t)(temp4);

    /* Configure sampling tmr to have 50us period */
    samplingTmrConfig.bCountingUp  = false;
    samplingTmrConfig.bPeriodic    = true;
    samplingTmrConfig.ePrescaler   = ADI_TMR_PRESCALER_16;
    samplingTmrConfig.eClockSource = tmr_clk;
    samplingTmrConfig.nLoad        = load_value;
    samplingTmrConfig.nAsyncLoad   = load_value;
    samplingTmrConfig.bReloading   = false;
    samplingTmrConfig.bSyncBypass  = false;
    eResult = adi_tmr_ConfigTimer(ADI_TMR_DEVICE_GP1, &samplingTmrConfig);

    if (eResult!=ADI_TMR_SUCCESS)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


void EnableGPTimer(void)
{
    /*GP TIMER ENABLE */
    eResult = adi_tmr_Enable(ADI_TMR_DEVICE_GP0, true);
}

void EnableSamplingTimer(void)
{
    eResult = adi_tmr_Enable(ADI_TMR_DEVICE_GP1, true);
}

void DisableGPTimer(void)
{
    /*GP TIMER DISABLE */
    eResult = adi_tmr_Enable(ADI_TMR_DEVICE_GP0, false);
}

void DisableSamplingTimer(void)
{
    /*Sampling TIMER DISABLE */
    eResult = adi_tmr_Enable(ADI_TMR_DEVICE_GP1, false);
}


/* GP0 timer callback function*/
void GP0CallbackFunction(void *pCBParam, uint32_t Event, void  * pArg)
{
    /* IF(Interrupt occurred because of a timeout) */
    if ((Event & ADI_TMR_EVENT_TIMEOUT) == ADI_TMR_EVENT_TIMEOUT)
    {
        gNumGp0Timeouts++;
        
        adi_gpio_Toggle(ADI_GPIO_PORT1, ADI_GPIO_PIN_12);
        waitDone = 1;
        timerTicks++;
        timerTicked = true;
    }
}


// NOTE: Could trigger ADC sampling here instead of having it blocking
void SamplingTimeCallbackFunction(void *pCBParam, uint32_t Event, void  * pArg)
{
    /* IF(Interrupt occurred because of a timeout) */
    if ((Event & ADI_TMR_EVENT_TIMEOUT) == ADI_TMR_EVENT_TIMEOUT)
    {
        /* Set adc read flag and reset timeout */
        check_ad7685 = true; // When this flag is set the data from the ADC will be read back over SPI
    }
}
