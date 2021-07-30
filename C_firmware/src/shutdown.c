
#include "shutdown.h"
#include <time.h>
#include <stddef.h>  /* for 'NULL' */
#include <stdio.h>   /* for scanf */
#include <string.h>  /* for strncmp */
#include <adi_pwr.h>
#include <adi_rtc.h>
#include "common.h"
#include "dn_uart.h"

#if 0
  #define DEBUG_PRINT(a) printf a
#else
  #define DEBUG_PRINT(a) (void)0
#endif

 /*Power control register access key */
#define ADI_PMG_KEY             (0x4859u)

/* set lowest interrupt priority (highest number)
   according to number of priority bits on-chip */
#define LOWEST_PRIORITY ((1U << __NVIC_PRIO_BITS) -1)

/* bump UART interrupt priority by one (lower number = higher priority)
   so that UART output continues during low-power hibernation test */
#define UART_PRIORITY (LOWEST_PRIORITY - 1)

/* leap-year compute macro (ignores leap-seconds) */
#define LEAP_YEAR(x) (((0==x%4)&&(0!=x%100))||(0==x%400))

/* Which RTC device to use for wakeup */
#define RTC_DEVICE_NUM_FOR_WUT 0

/* Number of RTC alarms required to be registered for successfull completion of the example */
#define ADI_RTC_NUM_ALARMS      1 

/* If the RTC needs to be calibrated */
#define ADI_RTC_CALIBRATE

#define RTC_ALARM_OFFSET 5

/* Trim interval */
#define ADI_RTC_TRIM_INTERVAL    ADI_RTC_TRIM_INTERVAL_14
/* Trim operation +/- */
#define ADI_RTC_TRIM_DIRECTION   ADI_RTC_TRIM_SUB
/* Trim  value */
#define ADI_RTC_TRIM_VALUE       ADI_RTC_TRIM_1

#define RTC_DEVICE_NUM    0

/* Device handle for RTC device0*/
extern ADI_RTC_HANDLE hDevice0;

// Allocate memory for RTC driver to own
static uint8_t aRtcDevMem0[ADI_RTC_MEMORY_SIZE];

/* Device handle for RTC device0*/
extern ADI_RTC_HANDLE hDevice0  = NULL;

/* Binary flag to indicate specified number of  RTC interrupt occurred */
volatile bool bRtcAlarmFlag;
/* Binary flag to indicate RTC interrupt occured */
volatile bool bRtcInterrupt;
/* Binary flag to indicate RTC-0  interrupt occured */
volatile bool bWutInterrupt;
/* Flag variable which is set to logic True when processor comes out of hibernation */
volatile uint32_t iHibernateExitFlag;
/* Alarm count*/
volatile uint32_t AlarmCount;


static uint32_t BuildSeconds(void)
{
    /* count up seconds from the epoc (1/1/70) to the most recent build time */

    char timestamp[] = __DATE__ " " __TIME__;
    int month_days [] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint32_t days, month = 1u, date, year, hours, minutes, seconds;
    char Month[4];

    /* parse the build timestamp */
    sscanf(timestamp, "%s %d %d %d:%d:%d", Month, (int *)&date,(int *)&year, (int *)&hours, (int *)&minutes, (int *)&seconds);

    /* parse ASCII month to a value */
    if     ( !strncmp(Month, "Jan", 3 )) month = 1;
    else if( !strncmp(Month, "Feb", 3 )) month = 2;
    else if( !strncmp(Month, "Mar", 3 )) month = 3;
    else if( !strncmp(Month, "Apr", 3 )) month = 4;
    else if( !strncmp(Month, "May", 3 )) month = 5;
    else if( !strncmp(Month, "Jun", 3 )) month = 6;
    else if( !strncmp(Month, "Jul", 3 )) month = 7;
    else if( !strncmp(Month, "Aug", 3 )) month = 8;
    else if( !strncmp(Month, "Sep", 3 )) month = 9;
    else if( !strncmp(Month, "Oct", 3 )) month = 10;
    else if( !strncmp(Month, "Nov", 3 )) month = 11;
    else if( !strncmp(Month, "Dec", 3 )) month = 12;


    /* count days from prior years */
    days=0;
    for (int y=1970; y<year; y++) {
        days += 365;
        if (LEAP_YEAR(y))
            days += 1;
    }

    /* add days for current year */
    for (int m=1; m<month; m++)
        days += month_days[m-1];

    /* adjust if current year is a leap year */
    if ( (LEAP_YEAR(year) && ( (month > 2) || ((month == 2) && (date == 29)) ) ) )
        days += 1;

    /* add days this month (not including current day) */
    days += date-1;

    return (days*24*60*60 + hours*60*60 + minutes*60 + seconds);
}

ADI_RTC_RESULT rtcInit (void) {

    uint32_t buildTime = BuildSeconds();
    ADI_RTC_RESULT eResult;

    /* Select and Enable Crystal as low frequency clock source */
    /* Crystal is required to wake up from full shutdown */
    adi_pwr_SetLFClockMux(ADI_CLOCK_MUX_LFCLK_LFXTAL);
    adi_pwr_EnableClockSource(ADI_CLOCK_SOURCE_LFXTAL, true);

    /* Use both static configuration and dynamic configuration for illustrative purpsoes */
    eResult = adi_rtc_Open(RTC_DEVICE_NUM, aRtcDevMem0, ADI_RTC_MEMORY_SIZE, &hDevice0);
    //DEBUG_RESULT("\n Failed to open the device %04d",eResult,ADI_RTC_SUCCESS);

    eResult = adi_rtc_RegisterCallback(hDevice0, rtc0Callback, hDevice0);
    //DEBUG_RESULT("\n Failed to register callback",eResult,ADI_RTC_SUCCESS);

    // This is counting up the seconds from 1970 to the most recent build
    // time and programming that as the count value... This is unnecessary
    // for Voyager, we just need to go X seconds in the future - real date
    // and time do not matter
    eResult = adi_rtc_SetCount(hDevice0, buildTime);
    //DEBUG_RESULT("Failed to set the count", eResult, ADI_RTC_SUCCESS);

    //DEBUG_RESULT("Failed to set the trim value",eResult,ADI_RTC_SUCCESS);
    eResult = adi_rtc_SetTrim(hDevice0, ADI_RTC_TRIM_INTERVAL, ADI_RTC_TRIM_VALUE, ADI_RTC_TRIM_DIRECTION);


    /* force a reset to the latest build timestamp */
    //DEBUG_MESSAGE("Resetting clock");
    eResult = adi_rtc_SetCount(hDevice0, buildTime);
    //DEBUG_RESULT("Failed to set count",eResult,ADI_RTC_SUCCESS);

    //DEBUG_MESSAGE("New time is:");
    //rtc_ReportTime();

    eResult = adi_rtc_Enable(hDevice0, true);
    //DEBUG_RESULT("Failed to enable the device",eResult,ADI_RTC_SUCCESS);


    eResult = adi_rtc_EnableInterrupts(hDevice0, ADI_RTC_ALARM_INT, true);
    //DEBUG_RESULT("Failed to enable interrupts",eResult,ADI_RTC_SUCCESS);

    return(eResult);
}


ADI_RTC_RESULT rtc_SetAlarm (uint32_t alarmTime_s) {

    /* Reset alarm count */
    ADI_RTC_RESULT eResult;
    uint32_t nRtc0Count;
    AlarmCount = 0;
    /* initialize flags */
    bRtcAlarmFlag = bRtcInterrupt = bWutInterrupt = false;

    //eResult = adi_rtc_SetTrim(hDevice0,ADI_RTC_TRIM_INTERVAL_6, ADI_RTC_TRIM_4, ADI_RTC_TRIM_ADD);
    //DEBUG_RESULT("Failed to set Trim value  ",eResult,ADI_RTC_SUCCESS);

    //eResult = adi_rtc_EnableTrim(hDevice0,true);
    //DEBUG_RESULT("\n Failed to enable Trim ",eResult,ADI_RTC_SUCCESS);

    eResult = adi_rtc_GetCount(hDevice0, &nRtc0Count);
    //DEBUG_RESULT("\n Failed to get count",eResult,ADI_RTC_SUCCESS);

    eResult = adi_rtc_SetAlarm(hDevice0, alarmTime_s + nRtc0Count);
    //DEBUG_RESULT("\n Failed to set alarm",eResult,ADI_RTC_SUCCESS);

    eResult = adi_rtc_EnableAlarm(hDevice0,true);
    //DEBUG_RESULT("Failed to enable alarm",eResult,ADI_RTC_SUCCESS);

    eResult = adi_rtc_EnableInterrupts(hDevice0, ADI_RTC_ALARM_INT, true);
    //DEBUG_RESULT("Failed to enable interrupts",eResult,ADI_RTC_SUCCESS);

    /* Any ISR that sets this flag will cause the core to wake up */
    iHibernateExitFlag = 0;

    /* go to sleep and await RTC ALARM interrupt */
    //DEBUG_MESSAGE("ALARM example starting at:");

    //rtc_ReportTime();
    return(eResult);
}

ADI_RTC_RESULT enterSleep(ADI_PWR_POWER_MODE low_power_mode_type)
{
  
    ADI_RTC_RESULT eResult;

    // Enable SRAM retention for hibernate
    pADI_PMG0->SRAMRET = 0x303;

    // HRM says to run on HFOSC before going into shutdown mode:
    adi_pwr_EnableClockSource(ADI_CLOCK_SOURCE_HFOSC, true);

    /* NOTE: Disable clocks to all peripherals before going to hibernate because it doesn't 
     * seem to be doing it itself (I can still see timerticks increment) */
    //pADI_CLKG0_CLK->CTL5 = 1<<BITP_CLKG_CLK_CTL5_PERCLKOFF;
    
    DEBUG_PRINT(("\nGoing to sleep at:"));
    //rtc_ReportTime();
    
    /* enter full hibernate mode with wakeup flag and no masking */
    /* Passing in a pointer to iHibernateExitFlag instead of a null pointer means
       that the sleep-on-exit feature is NOT being used */
    if (adi_pwr_EnterLowPowerMode(low_power_mode_type, &iHibernateExitFlag, 0))
    {
        DEBUG_MESSAGE("System Entering to Low Power Mode failed");
    }

    //DEBUG_MESSAGE("\nWaking up at:");
    //rtc_ReportTime();

    /* verify expected results */
    if (bWutInterrupt)
    {
        //DEBUG_RESULT("rtc_AlarmTest got unexpected WUT interrupt", bWutInterrupt, false);
        return(ADI_RTC_FAILURE);
    }
    if (!bRtcInterrupt)
    {
        DEBUG_MESSAGE("rtc_AlarmTest failed to get expected RTC interrupt");
        return(ADI_RTC_FAILURE);
    }
    if (!bRtcAlarmFlag)
    {
        DEBUG_MESSAGE("rtc_AlarmTest failed to get expected RTC ALARM interrupts");
        return(ADI_RTC_FAILURE);
    }
    /* disable alarm */
    eResult = adi_rtc_EnableAlarm(hDevice0, false);
    //DEBUG_RESULT("\n Failed to disable the device",eResult,ADI_RTC_SUCCESS);
    
    return eResult;
}

void wakeFromShutdown(void)
{
   /* Ungate peripheral clocks */
   pADI_CLKG0_CLK->CTL5 = 0;
   /* Re-enable Radio's UART IRQs */
   dn_uart_irq_enable();
   
   /* Enable ISR for RTC0 so that it can be executed on wakeup from shutdown. */
   /* It will be pending in the ISR since RTC0 triggered during shutdown */
   //rtcInit();

   pADI_PMG0->PWRKEY = ADI_PMG_KEY;

   /* Set PWRMOD back to 0*/
   pADI_PMG0->PWRMOD = 0u;  // Flexi Mode
   // pADI_PMG0->PWRMOD = ADI_PWR_MODE_ACTIVE;
}


/* RTC-0 Callback handler */
void rtc0Callback (void *pCBParam, uint32_t nEvent, void *EventArg) {

    bRtcInterrupt = true;

    /* process RTC interrupts (cleared by driver) */
    if( 0 != (ADI_RTC_WRITE_PEND_INT &  nEvent ))
    {
        DEBUG_MESSAGE("Got RTC interrupt callback with ADI_RTC_INT_SOURCE_WRITE_PEND status");
    }

    if( 0 != (ADI_RTC_WRITE_SYNC_INT & nEvent))
    {
        DEBUG_MESSAGE("Got RTC interrupt callback with ADI_RTC_INT_SOURCE_WRITE_SYNC status");
    }

    if( 0 != (ADI_RTC_WRITE_PENDERR_INT &  nEvent))
    {
        DEBUG_MESSAGE("Got RTC interrupt callback with ADI_RTC_WRITE_PENDERR_INT status");
    }

    if( 0 != (ADI_RTC_ISO_DONE_INT & nEvent))
    {
        DEBUG_MESSAGE("Got RTC interrupt callback with ADI_RTC_INT_SOURCE_ISO_DONE status");
    }

    if( 0 != (ADI_RTC_MOD60ALM_INT & nEvent))
    {
        DEBUG_MESSAGE("Got RTC interrupt callbackwithon ADI_RTC_INT_SOURCE_MOD60_ALARM status");
    }

    if( 0 != (ADI_RTC_ALARM_INT & nEvent))
    {
        /* Update alarm count */
        AlarmCount++;

        /* IF (Enough alarms registered) */
        if (AlarmCount >= ADI_RTC_NUM_ALARMS)
        {
            bRtcAlarmFlag = true;       /* note alarm flag */
            iHibernateExitFlag = 1   ;  /* exit hibernation on return from interrupt */
        }
        /* ELSE (more alarms needed) */
        else
        {
            /* Update RTC alarm */
            rtc_UpdateAlarm(60);
        }
    }

}

/*  standard ctime (time.h) constructs */
void rtc_ReportTime(void) {

    if (0)
    {
        char buffer[128];
        time_t rawtime;

        /* get the RTC count through the "time" CRTL function */
        time(&rawtime);
        /* print raw count */
        sprintf(buffer, "Raw time: %d", (int)rawtime);
        DEBUG_MESSAGE(buffer);

        /* convert to UTC string and print that too */
        sprintf(buffer, "UTC time: %s", ctime(&rawtime));
        DEBUG_MESSAGE(buffer);
    }
}


ADI_RTC_RESULT rtc_UpdateAlarm (uint32_t alarmTime_s) {
    ADI_RTC_RESULT eResult;
    uint32_t rtcCount;

    if(ADI_RTC_SUCCESS != (eResult = adi_rtc_GetCount(hDevice0,&rtcCount)))
    {
        //DEBUG_RESULT("\n Failed to get RTC Count %04d",eResult,ADI_RTC_SUCCESS);
        return(eResult);
    }
    if(ADI_RTC_SUCCESS != (eResult = adi_rtc_SetAlarm(hDevice0, rtcCount + alarmTime_s)))
    {
        //DEBUG_RESULT("\n Failed to set RTC Alarm %04d",eResult,ADI_RTC_SUCCESS);
        return(eResult);
    }
    return(eResult);
}
