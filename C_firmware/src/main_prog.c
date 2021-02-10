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

#include "ADC_channel_read.h"
#include "scheduler.h"
#include "SmartMesh_RF_cog.h"
#include "dn_uart.h"
#include "SPI0_ADXL362.h"
#include "SPI1_AD7685.h"  

/*=======================  D E F I N E S   ===================================*/
/* Clocks */ 
#define HFOSC   26000000
#define LFOSC   32768
#define LFXTAL  32768
#define HFXTAL  26000000
#define HCLOCK  26000000   //AHB Clock (Core clock)
#define PCLOCK  26000000   //Peripheral Clock  
#define ACLOCK  6500000    //ADC Clock (Default 6.5MHz)

/* User */ 
typedef enum
{
    NEW_PARAM,
    ACQ,
    CALC,
    TX,
    WAIT
} state_t; //Data handling states


/*=======================  D A T A   =========================================*/
scheduler_t          main_scheduler;
scheduler_t_us       sampling_scheduler;
app_vars_t           app_vars;

state_t              state = NEW_PARAM;
int                  waitDone = 0;
int                  timerTicks = 0;
double               cpu_time_used;

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

/* ADC Sampling Parameters */    
uint16_t             adcNumSamples = ADC_NUM_SAMPLES;
uint32_t             adcSampFreq;
uint32_t             FFT_Samples_Timeout = 1;

uint32_t             adcExtraBits=12; 
uint32_t             adcSampTime;

/* Mote Parameters */
dn_ipmt_setParameter_networkId_rpt* my_reply;
dn_err_t                            my_error;
uint16_t                            NET_ID;
uint8_t                             join_duty=255;     //Requested duty cycle for join attempts (see SmartMESH documentation)
uint32_t                            ms_per_packet=500;  //Requested SmartMESH bandwidth (see SmartMESH documentation)

/*=======================  P R O T O T Y P E S   =============================*/
/* System */
int32_t   adi_initpinmux(void);
void      StartScheduler(scheduler_t);
void      StartSamplingScheduler(scheduler_t_us); 

/*=========================== main ===========================================*/
int main(void)
{
   int timerTickLimit;

   /* Pinmux initialization. */
   adi_initpinmux();   
   /* Power initialization. */
   adi_pwr_Init();              
   /* System clock initialization. */
   adi_pwr_SetClockDivider(ADI_CLOCK_HCLK, HDIV);
   /* Peripheral clock initialization. */
   adi_pwr_SetClockDivider(ADI_CLOCK_PCLK, PDIV);
   /* ADC clock initialization. */
   adi_pwr_SetClockDivider(ADI_CLOCK_ACLK, ADIV);
   /* gpio initialization. */
   adi_gpio_Init(gpioMemory, ADI_GPIO_MEMORY_SIZE);  
   
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
   
   /* Debug GPIO Setup */
#ifndef COG
   adi_gpio_OutputEnable( ADI_GPIO_PORT0, ADI_GPIO_PIN_10, true);
   adi_gpio_SetLow( ADI_GPIO_PORT0, ADI_GPIO_PIN_10);
#endif
   /* Initialise ADC */
   ADC_Init(); 

#ifndef OLD_MOTE
   /* Initialise AD7685 */
   ad7685init();
#endif

  /* Reset local variables */
   memset(&app_vars, 0, sizeof(app_vars));
   
   /* dn_ipmt_init initialises all the variables necessary for the application to function
   This function results in the call of dn_uart_init which initalises the UART of the MCU */ 
   dn_ipmt_init(
      dn_ipmt_notif_cb,                /* Notification Call Back */
      app_vars.notifBuf,               /* Notification Buffer */
      sizeof(app_vars.notifBuf),       /* Notification Buffer Length */
      dn_ipmt_reply_cb                 /* Reply call back */
        );
  
   /* Scheduler initialisation and Start */
   main_scheduler.source=GP_TMR;  //NOTE only GP_TMR source available for now
   main_scheduler.tick_ms=500;
   StartScheduler(main_scheduler);
   // 2 min. timeout
   timerTickLimit = (int)(120000.0f/main_scheduler.tick_ms + 0.5);
   
   /* Scheduler used for FFT sampling */ 
   sampling_scheduler.source=GP_TMR;  //NOTE only GP_TMR source available for now
   sampling_scheduler.tick_us=SAMPLING_SCHEDULER_CLOCK_USEC;
   StartSamplingScheduler(sampling_scheduler);
   
   /* Communication State Machine */
   while (1)
   {
       while(head != tail)
       {
           buffer_handle();                                                     /* here the buffer is checked for incoming data and processed */
       }

       if(!awaiting_response)                                                   /*Schedule the next event only if not awaiting response, or stuck in the middle of data tranmission*/
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
           scheduleEvent(&api_response_timeout);
           timerTicks = 0;
       }
       
       /* Data handling state machine */
       switch (state)
       {    
           case NEW_PARAM:
#ifndef OLD_MOTE
               adcSampFreq = getSampFreq();
               FFT_Samples_Timeout = getSampTimeout(adcSampFreq);
#else 
               adcExtraBits = getExtraBits();
               adcSampTime = getSamptime(ACLOCK, getResolution());
#endif
               state = ACQ;
               break;
               
           case ACQ:
#ifndef OLD_MOTE
               ad7685_SampleData_Blocking();
#else
               ADC_SampleData_Blocking_Oversampling(adcExtraBits, adcSampTime);
#endif
               state = CALC;
               break;

           case CALC:
               ADC_Calc_FFT(ADC_NUM_SAMPLES);
               startTx();
               state = TX;
               break;

           case TX:
               if (!txRunning())
                   state = WAIT;
               break;

           case WAIT:
               if (waitDone)
               {
                   waitDone = 0;
                   state = NEW_PARAM;
               }
               break;

           default:
               while (1);
               break;
       }
   }
}
