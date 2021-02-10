/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors. 
By using this software you agree to the terms of the associated Analog Devices 
License Agreement.
*********************************************************************************/
/*!
* @file      scheduler.h
* @brief     Main header file for scheduler.c
*
* @details
*            
*
*/

/*=============  I N C L U D E S   =============*/
#include <stdint.h>
#include <common.h>

#include <adi_processor.h>
#include <drivers/pwr/adi_pwr.h>
#include <drivers/tmr/adi_tmr.h>

/*=============  TYPEDEFS  =============*/
typedef enum 
{
  RTC,
  GP_TMR,
}sched_source;

typedef struct 
{
  sched_source source;
  uint16_t tick_ms;
} scheduler_t;

typedef struct 
{
  sched_source source;
  uint16_t tick_us;
} scheduler_t_us;

/*=============  DEFINES  =============*/
#define GP0_LOAD_VALUE_FOR_10MS_PERIOD (4063u)
#define GP0_LOAD_VALUE_FOR_500MS_PERIOD (50781u)
#define TMR_FACTOR                     (4u)


#if defined(__ADUCM302x__)
#define GP_TMR_CAPTURE_EVENT 14u
#elif defined(__ADUCM4x50__)
#define GP_TMR_CAPTURE_EVENT 16u
#else
#error TMR is not ported for this processor
#endif

/*=============  DATA  =============*/
#define SAMPLING_SCHEDULER_CLOCK_USEC 10

extern bool check_ad7685;

/*=============  L O C A L    F U N C T I O N S  =============*/



/*=============  C O D E  =============*/
