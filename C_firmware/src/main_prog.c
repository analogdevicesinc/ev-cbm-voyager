/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated Analog Devices
License Agreement.
*********************************************************************************/

/*!
* @file      main_prog.c
* @brief     main body of SmartMesh condition-based monitoring application
*
* @details
*           initialises the hardware used in the application and controls
*           the state machines for data handling and communication
*
*/

/*=======================  I N C L U D E S   =================================*/
#include <drivers/pwr/adi_pwr.h>
#include <drivers/gpio/adi_gpio.h>
#include <drivers/spi/adi_spi.h>         /* 02/27/2019  Fan added */
#include <drivers/dma/adi_dma.h>         /* 02/27/2019  Fan added */
#include <arm_math.h>
#include <time.h>

#include "scheduler.h"
#include "SmartMesh_RF_cog.h"
#include "dn_uart.h"
#include "SPI0_ADXL362.h"
#include "SPI1_AD7685.h"
#include "shutdown.h"
#include "ext_flash.h"

// For printf statements
#include "stdio.h"
#include "stdint.h"

// For ITM events
//#include "arm_itm.h"
//#define ITM_PROFILING (1)

/*=======================  D E F I N E S   ===================================*/

#if 0
  #define DEBUG_PRINT(a) printf a
#else
  #define DEBUG_PRINT(a) (void)0
#endif


/* Clocks */
#define HFOSC   26000000
#define LFOSC   32768
#define LFXTAL  32768
#define HFXTAL  26000000
#define HCLOCK  26000000   //AHB Clock (Core clock)
#define PCLOCK  26000000   //Peripheral Clock
#define ACLOCK  6500000    //ADC Clock (Default 6.5MHz)

//#define COG true

/* User */
typedef enum
{
    RADIO_SETUP,
    WAIT_FOR_READY,
    NEW_PARAM,
    ACQ,
    GET_DATA,
    CALC,
    TX,
    WAIT,
    SLEEP_MCU,
    WAKEUP
} state_t; //Data handling states


/*=======================  D A T A   =========================================*/
scheduler_t          main_scheduler;
scheduler_t_us       sampling_scheduler;
app_vars_t           app_vars;

state_t              state      = RADIO_SETUP;
int                  waitDone   = 0;
int                  timerTicks = 0;
int                  timerTicksPrev = 0;
double               cpu_time_used;
int                  timeoutLimit_ms;
int                  timerTickLimit;
int                  numTimeouts_DBG  = 0; // Keep track of timeouts for debug purposes
int                  numTxSuccess_DBG = 0; // Keep track of successfull TXs for debug purposes
uint32_t             sleepDur_s       = 0; // Get the desired sleep duration from the GUI

extern uint32_t lastRtcCount = 0;

/* Buffer variables */
extern int           head,tail;
extern uint8_t       buffer_uart[buffer_size];

/* Flag_Check initialization */
extern State_Check   Flag_Check;

/*Memory for the GPIO driver*/
static uint8_t       gpioMemory[ADI_GPIO_MEMORY_SIZE];

/* Clock frequency dividers */
uint16_t             HDIV=(uint16_t)(HFOSC/HCLOCK);
uint16_t             PDIV=(uint16_t)(HFOSC/PCLOCK);
uint16_t             ADIV=(uint16_t)(HFOSC/ACLOCK);
uint32_t             hfosc_freq=HFOSC;
uint32_t             pclk_freq=PCLOCK;
uint32_t             lfosc_freq=LFOSC;
uint32_t             hfxtal_freq=HFXTAL;
uint32_t             lfxtal_freq=LFXTAL;
uint32_t             aclock_freq=ACLOCK;

/* Flags */
bool                 idle=false;
bool                 awaiting_response=false;
bool                 samples_acquired=false;
bool                 include_tx_hdr=true;
bool                 send_axis_hdr[3];

/* ADC Sampling Parameters */
uint32_t             adcNumSamples = 0;
uint32_t             adcSampFreq;
uint32_t             FFT_Samples_Timeout = 1;

uint32_t             adcExtraBits=12;
uint32_t             adcSampTime_us;

/* Mote Parameters */
dn_ipmt_setParameter_networkId_rpt* my_reply;
dn_err_t                            my_error;
uint16_t                            NET_ID;
uint8_t                             join_duty=255;      // Requested duty cycle for join attempts (see SmartMESH documentation)
uint32_t                            ms_per_packet=500;  // Requested SmartMESH bandwidth (see SmartMESH documentation)(called inter-packet interval)
                                    // TODO: should ms_per_packet be increased before going to sleep for battery?
bool                                radioSetupDone = false;

axis_t      active_axis_tx;
uint32_t    numSamplesRemaining[3];


/*=======================  P R O T O T Y P E S   =============================*/
/* System */
int32_t   adi_initpinmux(void);
void      StartScheduler(scheduler_t);
void      StartSamplingScheduler(scheduler_t_us);
void      usleep(uint32_t);
void      setupRadio(void);
void      initialise();
state_t   getState();
//void ledDance();

uint32_t inter_packet_interval;

bool     all_data_sent = false;

/*=========================== main ===========================================*/
int main(void)
{
   initialise();

   wakeFromShutdown();
   dn_uart_irq_enable();
   DEBUG_PRINT(("Start of Main\n"));


   /* Enable ISR for RTC1 so that it can be executed on wakeup from shutdown. */
   /* It will be pending in the ISR since RTC0 triggered during shutdown */
   rtcInit();

   /* Scheduler initialisation and Start */
   main_scheduler.source=GP_TMR;  //NOTE only GP_TMR source available for now
   main_scheduler.tick_ms=10000;    // Any time > 500ms will use LF_CLK
   StartScheduler(main_scheduler);

   // Define timeout
   timeoutLimit_ms = 90e3*1;  // 60,000ms in a minute
   if (main_scheduler.tick_ms >= timeoutLimit_ms)
        DEBUG_PRINT(("The timeout value needs to be larger than the Main Scheduler period"));

   // Calculate how many ticks of the Main Scheduler need to occur, while
   // not getting a response, to cause a timeout
   timerTickLimit = (int)(timeoutLimit_ms/main_scheduler.tick_ms + 0.5);

   //ITM_EVENT8(1, 0x1);
   
   initExtFlashSPI();
   prepareFlash();
   
   /* Communication State Machine */
   while (1)  // Both FSMs run in this forever loop
   {
       while(head != tail)
       {
           buffer_handle();                                                     /* here the buffer is checked for incoming data and processed */
       }

       if(!awaiting_response && radioSetupDone)                                 /*Schedule the next event only if not awaiting response, or stuck in the middle of data tranmission*/
       {
           switch(Flag_Check)
           {
               case Mote_Status:
                   scheduleEvent(&api_getMoteStatus);                           /* Function call for Mote Status is done here */
                   break;

               case Open_Socket:
                   scheduleEvent(&api_openSocket);                              /* Function call for opening a socket is done here */
                   break;

               case Bind_Socket:
                   scheduleEvent(&api_bindSocket);                              /* Function call for binding the socket is done here */
                   break;

               case Set_NetID:
                   scheduleEvent(&api_setNetworkID);                            /* Function call for setting the network ID is done here */
                   break;

               case Get_NetID:
                   scheduleEvent(&api_getNetworkID);                            /* Function call for getting the network ID is done here */
                   break;

               case Set_JoinDC:
                   scheduleEvent(&api_setJoinDutyCycle);                        /* Function call for setting the Join Duty Cycle is done here */
                   break;

               case Join:
                   scheduleEvent(&api_join);                                    /* Function call for joining the network is done here */
                   break;

               case Request_Service:
                   scheduleEvent(&api_requestService);
                   break;

               case Get_Service_Info:
                   scheduleEvent(&api_getServiceInfo);
                   break;

               case SendTo:
                   scheduleEvent(&api_sendTo);                                  /* Function call for Data packet tranmission is done here */
                   break;

               default:
                   break;
           }

           timerTicks = 0;
       }

       if (timerTicks >= timerTickLimit)
       {
           numTimeouts_DBG++;
           DEBUG_PRINT(("T.O. #%d\n", numTimeouts_DBG));
           scheduleEvent(&api_response_timeout);
           timerTicks = 0;
       }

       if (timerTicked && ((state == WAIT_FOR_READY) || (state == NEW_PARAM)))
       {
           // Let manager know you're ready
           timerTicked = false;
           scheduleEvent(&sendMgrReady);
       }

       /* Data handling state machine */
       switch (state)
       {
           case RADIO_SETUP:
               if (!radioSetupDone)
               {
                   setupRadio();
               }
#ifndef COG
               if (Flag_Check == SendTo)
#else                    
               if (radioSetupDone)
#endif
               {  
                  /* Wait for radio to become connected before moving to 
                   * get new parameters */
                 state = WAIT_FOR_READY;
               }
               break;

           case WAIT_FOR_READY:
               // Wait for the Python code on manager to be 
               // up and running before progressing
               if (getMgrReady())
               {
                   state = NEW_PARAM;
               }
               break;


           case NEW_PARAM:
#ifndef OLD_MOTE

               if (getMgrReady())
               {
                   // Manager will send a ready signal after every full frame is received. 
                   // If manager SW is closed then sampling will stop
                   clearMgrReady();

                   adcSampFreq    = getSampFreq();
                   adcSampTime_us = getSampTime_us(adcSampFreq);
                   adcNumSamples  = getAdcNumSamples();
                   sleepDur_s     = getSleepDur();

                   if (adcNumSamples > ADC_SAMPLES_PER_BUFF) 
                   {
                       ext_flash_needed = true;   
                       send_axis_hdr[x_active] = 1;
                       send_axis_hdr[y_active] = 1;
                       send_axis_hdr[z_active] = 1;
                   } 
                   else
                   {
                       ext_flash_needed = false;
                       send_axis_hdr[x_active] = 0;
                       send_axis_hdr[y_active] = 0;
                       send_axis_hdr[z_active] = 0;
                   }
                   
                   updateAdcParams(adcNumSamples, ext_flash_needed);

                   numSamplesRemaining[x_active] = adcNumSamples;
                   numSamplesRemaining[y_active] = adcNumSamples;
                   numSamplesRemaining[z_active] = adcNumSamples;

                   /* Initialise ADC */
                   /* This was moved into FSM because it depends on num samples being taken */
                   ADC_Init(); 
#else
                   adcExtraBits = getExtraBits();
                   adcSampTime_us = getSamptime(ACLOCK, getResolution());
#endif
                   /* Turn on Sampling Timer before going to ACQ*/
                   sampling_scheduler.source  = GP_TMR;    //NOTE only GP_TMR source available for now
                   sampling_scheduler.tick_us = adcSampTime_us;
                   StartSamplingScheduler(sampling_scheduler);

                   state = ACQ;
               }
               break;


           case ACQ:
               if (!samples_acquired)
               {
                   /* Initialise AD7685 */
                   ad7685init();
                  DEBUG_PRINT(("Acq..."));
#ifndef OLD_MOTE
                  ad7685_SampleData_Blocking();
#else
                  ADC_SampleData_Blocking_Oversampling(adcExtraBits, adcSampTime_us);
#endif
                  /* Turn off sampling scheduler after all samples have been taken */
                  DisableSamplingTimer();

                  samples_acquired = true;
               }

               if (!ext_flash_needed)
               {
                  state = CALC;
               } 
               else if (ext_flash_needed && pollFlashStatus(BITP_FLSH_STAT_OIP, false))
               { 
                  // If flash is needed check that no operation is in progress
                  // before progressing
                  waitForSpi2();
                  state = GET_DATA;
               }
               break;


           case GET_DATA:
               // This state will only ever be entered if we need flash
               if (!checkPagePointers(x_active))
                   active_axis_tx = x_active;
               else if (!checkPagePointers(y_active))
                   active_axis_tx = y_active;
               else if (!checkPagePointers(z_active))
                   active_axis_tx = z_active;

               DEBUG_PRINT(("Fetching data from axis %d - Block %d, Page %d\n", active_axis_tx, getBlockAddrRd(active_axis_tx), getPageAddrRd(active_axis_tx)));

               flashPageRead(getBlockAddrRd(active_axis_tx), getPageAddrRd(active_axis_tx));
               // Only going to be dealing with one axis at a time so 
               // just reuse the x axis array.
               // Read from column address 0x0 and into location 1 of data array. 
               // Location 0 is reserved for the GUI header
               
               // TODO replace FLASH_PAGE_SIZE_B with num samples remaining... doesn't really matter
               flashReadFromCache(0x0, (uint8_t*)&adcDataX[1], FLASH_PAGE_SIZE_B);
               updatePagePointers(false, active_axis_tx);  // false = read
               
               updateAdcParams(numSamplesRemaining[active_axis_tx], ext_flash_needed);
               numSamplesRemaining[active_axis_tx] -= ADC_SAMPLES_PER_BUFF;

               // HACK: Need to copy index 1025 into 1026. This has to do with the fact that
               // the GUI expects the ADC header to be 4 bytes instead of the correct 2.
               // But when you try to change its expectations it falls over
               // So duplicating the final byte in the first flash page for now
               adcDataX[ADC_DATA_END_1ST_S] = adcDataX[ADC_DATA_END_1ST_S-1];

               state = CALC;
               break;

           case CALC:
               DEBUG_PRINT(("Calc..."));
               samples_acquired = false;  // Reset flag

               if (ext_flash_needed)
               {
                  // Can't do FFT if using off-chip flash. FFT needs to be performed on 
                  // all of the the data at once
                  startTx(send_axis_hdr[active_axis_tx], active_axis_tx);
                  send_axis_hdr[active_axis_tx] = 0;  // Header sent for current axis
               } 
               else
               {
                  ADC_Calc_FFT();
                  startTx(true, NULL);
               }

               //rtc_ReportTime();
               state = TX;
               break;

           case TX:
#ifndef COG
               if (!txRunning() && gotFinalAck()) 
#else
               if (1)
#endif
               {
                   numTxSuccess_DBG++;

                   if (!checkAllPagePointers() && ext_flash_needed)
                   {
                       state = GET_DATA;
                   }
                   else if (sleepDur_s == 0)
                   {
                      DEBUG_PRINT(("Finished Tx #%d\n", numTxSuccess_DBG));
                      //rtc_ReportTime();
                      state = WAIT;
                   }
                   else
                   {
                      state = SLEEP_MCU;
                      DEBUG_PRINT(("Finished TX #%d. Going to sleep\n", numTxSuccess_DBG));
                   }
               }
               break;

           case WAIT:
               if (waitDone)
               {
                   waitDone = 0;
                   timerTicked = false;
                   state = NEW_PARAM;
               }
               break;


           case SLEEP_MCU:
              rtc_SetAlarm(sleepDur_s);
              adi_gpio_SetLow( ADI_GPIO_PORT1, ADI_GPIO_PIN_12);
              
              //enterSleep(ADI_PWR_MODE_SHUTDOWN);
              enterSleep(ADI_PWR_MODE_HIBERNATE);

              state = WAKEUP;
              break;

           case WAKEUP:
              wakeFromShutdown();  
              adi_gpio_SetHigh( ADI_GPIO_PORT1, ADI_GPIO_PIN_12);
              timerTicked = false;
              
              state = NEW_PARAM;
              break;

           default:
               while (1);
               break;
       }
   }
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

void setupRadio()
{
   /* Reset the radio so that it exits its DeepSleep mode (if in it)
    * GPIO14 (P0_14) is connected to the radio's reset port. Pull low to reset */
   adi_gpio_SetLow( ADI_GPIO_PORT0, ADI_GPIO_PIN_14);
   /* Give it some time */
   usleep(100);
   adi_gpio_SetHigh( ADI_GPIO_PORT0, ADI_GPIO_PIN_14);
   usleep(100);

    /* Reset local variables */
   memset(&app_vars, 0, sizeof(app_vars));

   /* dn_ipmt_init initialises all the variables necessary for the application to function
   This function results in the call of dn_uart_init which initalises the UART of the MCU */
   dn_ipmt_init
   (
      dn_ipmt_notif_cb,                /* Notification Call Back */
      app_vars.notifBuf,               /* Notification Buffer */
      sizeof(app_vars.notifBuf),       /* Notification Buffer Length */
      dn_ipmt_reply_cb                 /* Reply call back */
   );

   radioSetupDone = true;
}


void initialise()
{

   /* Pinmux initialization. */
   adi_initpinmux();
   
   /* Power initialization. */
   adi_pwr_Init();
   /* System clock initialization. Note, LFOSC is always, it cannot be turned off*/
   adi_pwr_SetClockDivider(ADI_CLOCK_HCLK, HDIV);
   /* Peripheral clock initialization. */
   adi_pwr_SetClockDivider(ADI_CLOCK_PCLK, PDIV);
   /* ADC clock initialization. */
   adi_pwr_SetClockDivider(ADI_CLOCK_ACLK, ADIV);

   /* gpio initialization. */
   adi_gpio_Init(gpioMemory, ADI_GPIO_MEMORY_SIZE);

   /* Set GPIO14 (P0_14) as a controllable GPIO output. 
    * This pin is connected to Radio's reset, which is needed
    * to bring it out of DeepSleep mode. Radio is reset when GPIO14 goes low */
   adi_gpio_OutputEnable( ADI_GPIO_PORT0, ADI_GPIO_PIN_14, true);
   adi_gpio_SetHigh( ADI_GPIO_PORT0, ADI_GPIO_PIN_14);

   /* Enabling PWM mode */
   adi_gpio_OutputEnable( ADI_GPIO_PORT2, ADI_GPIO_PIN_0, true);                /* ADP5300 PWM GPIO (COG Board) */
   adi_gpio_SetHigh( ADI_GPIO_PORT2, ADI_GPIO_PIN_0);                           /* enabling ADP5300 PWM mode */
   /* Enable ADP198, Enable VBAT_ACC Supply, P1_14 */
   adi_gpio_OutputEnable( ADI_GPIO_PORT1, ADI_GPIO_PIN_14, true);
   adi_gpio_SetHigh( ADI_GPIO_PORT1, ADI_GPIO_PIN_14);
   /* Enable ADP198, Enable +3.5V_ACC Supply, P2_09 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_9, true);
   adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_9);
   /* Enable LTC3129 RUN, Output +3.5V_ACC, P2_08 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_8, true);
   adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_8);
   /* Enable VBAT_NAND Supply, P2_04 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_4, true);
   adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_4);

#ifndef OLD_MOTE
   /* Set AD7685 */
   /* Disable CONV, P2_01 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_1, true);
   adi_gpio_SetLow(ADI_GPIO_PORT2, ADI_GPIO_PIN_1);

   /* Set ADXL356B */
   /* Select 20g full scale, P2_02 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_2, true);
   adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_2);
   /* Enable Measurement Mode, P2_05 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_5, true);
   adi_gpio_SetHigh(ADI_GPIO_PORT2, ADI_GPIO_PIN_5);
   /* Disable SELF TEST ST1, P2_07 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_7, true);
   adi_gpio_SetLow(ADI_GPIO_PORT2, ADI_GPIO_PIN_7);
   /* Disable SELF TEST ST2, P2_06 */
   adi_gpio_OutputEnable(ADI_GPIO_PORT2, ADI_GPIO_PIN_6, true);
   adi_gpio_SetLow(ADI_GPIO_PORT2, ADI_GPIO_PIN_6);
#endif

   /* Enable Green LED */
   adi_gpio_OutputEnable( ADI_GPIO_PORT1, ADI_GPIO_PIN_12, true);
   adi_gpio_SetLow( ADI_GPIO_PORT1, ADI_GPIO_PIN_12);

   /* Enable Red LED outputbut set low */
   adi_gpio_SetLow( ADI_GPIO_PORT1, ADI_GPIO_PIN_13);
   adi_gpio_OutputEnable( ADI_GPIO_PORT1, ADI_GPIO_PIN_13, true);

   /* Debug GPIO Setup */
#ifndef COG
   adi_gpio_OutputEnable( ADI_GPIO_PORT0, ADI_GPIO_PIN_10, true);
   adi_gpio_SetLow( ADI_GPIO_PORT0, ADI_GPIO_PIN_10);
#endif

#ifndef OLD_MOTE
   /* Initialise AD7685 */
   // This needed to be moved into main FSM. Needed to be called
   // everytime after waking up
   //ad7685init();
#endif
}


