
#ifndef SHUTDOWN__
#define SHUTDOWN__

#include <time.h>
#include <stddef.h>  /* for 'NULL' */
#include <stdio.h>   /* for scanf */
#include <string.h>  /* for strncmp */
#include <drivers/pwr/adi_pwr.h>
#include <drivers/rtc/adi_rtc.h>
#include "common.h"

/* prototypes for RTC initialization */
ADI_RTC_RESULT rtcInit(void);
/* prototypes for RTC calibration runctions  */
ADI_RTC_RESULT rtc_Calibrate(void);
/* prototypes for RTC updating the count  */
ADI_RTC_RESULT rtc_UpdateAlarm (uint32_t alarmTime_s);
/* prototypes for RTC test function  */
ADI_RTC_RESULT rtc_SetAlarm(uint32_t alarmTime_s);
/* prototypes for RTC time reporting function */
void rtc_ReportTime(void);
/* prototypes for RTC seconds computation */
uint32_t BuildSeconds(void);
/* prototypes for clock initialization */
ADI_PWR_RESULT InitClock(void);

/* Enter defined power mode */
ADI_RTC_RESULT enterSleep(ADI_PWR_POWER_MODE);

void wakeFromShutdown(void);

/* callbacks */
void rtc0Callback (void *pCBParam, uint32_t nEvent, void *EventArg);

#endif  // SHUTDOWN__
