/*! *****************************************************************************
 * @file    SmartMesh_RF_cog.c
 * @brief   Application software for handling data transfer from MCU cog to module(DC9018B-B04,DC9018A-B and DC9021A-05) and vice-versa.
 * @details The APIs in this file can only be called from MCU cog.
 *
 * @note    This application only facilitates continuos transfer of data from the module to manager of the network.
 -----------------------------------------------------------------------------
Copyright (c) 2017-2018 Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  - Modified versions of the software must be conspicuously marked as such.
  - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
  - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
  - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
  - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-
INFRINGEMENT, TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF
CLAIMS OF INTELLECTUAL PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

!*/

/*=======================  I N C L U D E S   =================================*/

#include <stdint.h>
#include <stdio.h>
#include <drivers/i2c/adi_i2c.h>
#include <common.h>
#include <adi_processor.h>
#include <math.h>

#include "ADC_channel_read.h"
#include "dn_ipmt.h"
#include "dn_uart.h"
#include "SmartMesh_RF_cog.h"
#include "scheduler.h"
#include "ext_flash.h"


/*=======================  D E F I N E S   ===================================*/
#if 0
  #define DEBUG_PRINT(a) printf a
#else
  #define DEBUG_PRINT(a) (void)0
#endif

/*=======================  V A R I A B L E S =================================*/

State_Check                 Flag_Check = Boot_Status;

/* Buffer variables */
uint8_t                     buffer_uart[buffer_size];
int                         head,tail,next,max_len;

/* Delay Variable */
uint32_t                    delay_count=0;

/* Data transfer Flags */
extern bool                 awaiting_response;
bool                        more_data_to_send = false;
bool                        retry_data = false;

/* Control flags */
bool                        empty,full;

/* I2C variables */
extern uint8_t              devMem[ADI_I2C_MEMORY_SIZE];
extern ADI_I2C_HANDLE       i2cDevice;
extern uint8_t              prologueData[5];
extern ADI_I2C_TRANSACTION  xfr;
extern ADI_I2C_RESULT       result;
extern uint32_t             hwError_i2c;
extern uint8_t              rxData[DATASIZE_i2c];

/* Python Downstream variables*/
bool                         mgrReady = false;
static uint16_t              cmdDescriptor;
static uint32_t              samp_frequency   = 512;
static uint16_t              extra_bits_var   = 12;
static uint8_t               alarm            = 0;
static uint8_t               samp_time;
static uint8_t               axis_info        = 111;
static int txRun    = 0;
static int finalAck = 0;  // Set when api_sendTo_reply() occurs for the final packet in a frame

static uint32_t              sleep_dur_s     = 0;
static uint32_t              adcNumSamples   = 512;
//

/* Version Number to Match Firmware and GUI */
static uint8_t               int_version = (uint8_t)((VERSION)*100 - ((uint8_t)VERSION)*100);
 
/* Application variables struct for SmartMesh */
extern app_vars_t           app_vars;

/* Mote Status */
Mote_Status_t               MoteStatus=Booting;

/* Mote Bandwidth Parameter */
extern uint32_t             ms_per_packet;

/* Notification Packet for python GUI */
uint8_t                     sensor_Value_temp[27] ={0x00,0x01,0x05,0x00,0xff,
                                                    0x01,0x05,0x00,0x00,0x00,
                                                    0x00,0x3d,0x22,0xdd,0x59,
                                                    0x00,0x0c,0xea,0x46,0x00,
                                                    0x00,0x75,0x30,0x01,0x10};

/*Data being sent to manager*/
uint8_t                    data_counter=0;

/*Network ID*/
extern uint16_t NET_ID;
extern uint8_t join_duty;

extern uint32_t            *pVbat;
uint32_t                   Vbat;

/* Packet Sent Flag */
static int txPacketDone = 1;

/*=======================  I N C L U D E S   =================================*/

/* event */
void      cancelEvent(void);
void      setCallback(reply_callback cb);

/* app */
void      Smartmesh_RF_cog_receive(void);


/*=========================== Buffer handle ==================================*/
/**
 * @brief    Data written to the cicular buffer is handled here.
 *
 * @param       void
 *
 * @return  void
 *
 * Retrieves the data written to the circular buffer by the interrupt handler and processes the data.
 *
 * @sa      Smartmesh_RF_cog_receive().
 *
 * @note    Buffer handle is called whenever data is written to the circular buffer.
 */

void buffer_handle(void)
{
  while(head!=tail)
  {
   next = tail+1;

   if(next>=max_len)
      next = 0;

   Smartmesh_RF_cog_receive();
   tail = next;
  }
}


/*=========================== Schedule Event =================================*/
/**
 * @brief    Schedules events to be executed.
 *
 * @param[in]   cb      Pointer to the function to be scheduled.
 *
 * @return  void.
 *
 * This function currently executes the event that was scheduled.
 *
 * @sa     app_vars.eventCb().
 *
 * @note    This function is provided for implementing time delays using RTC
 *          interrupts, currently the application is developed using software delays.
 */

void scheduleEvent(timer_callback cb)
{
  /* select function to call */
   app_vars.eventCb = cb;
   app_vars.eventCb();
}

/*=========================== Cancel Event ===================================*/
/**
 * @brief    Clears function call.
 *
 * @param       void.
 *
 * @return  void.
 *
 * Clears the function call pointer to NULL.
 *
 * @note    NA.
 */

void cancelEvent(void)
{
   /* clear function to call */
   app_vars.eventCb = NULL;
}

/*=========================== Set Callback ===================================*/
/**
 * @brief    Sets the reply callback function.
 *
 * @param [in]  cb      Pointer to the function to be called upon receiving reply.
 *
 * @return  void.
 *
 * Function to be called on receving reply from the API is set here.
 *
 * @note    NA.
 */

void setCallback(reply_callback cb)
{
  /* set callback */
  app_vars.replyCb = cb;
}

/*=========================== Ipmt ===========================================*/
/**
 * @brief    Notifications are handled here.
 *
 * @param [in] cmdId      command ID.
 * @param [in] subCmdId   sub command ID.
 *
 * @return  void.
 *
 * All the notifications from the module are handled here,
 * there are 5 types of notifications namely:-
 *  1)Time information.
 *  2)Events.
 *  3)Packet received.
 *  4)Transmit done.
 *  5)Received advertisement.
 *
 *
 * @sa          dn_ipmt_cancelTx()
 *
 *
 * @note    This handles only Event notifications, provisions are given to
            handle other notifications as well. For more information on the
            various fields present in the notifications received
            refer SmartMesh_IP_Mote_Serial_API_Guide.pdf .
 */

void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId)                          /* notification call-back */
{
  dn_ipmt_events_nt* dn_ipmt_events_notif;
  dn_ipmt_receive_nt* dn_ipmt_receive_notif;

  /*Declaring variables for CMDID_receive*/
  char           sampFrequencyArray[8];
  char           alarmArray[8];
  char           axisArray[8];
  char           numSampArray[8];
  char           sleepDurArray[8];
  char           cmdDescriptorArray[8];

  switch (cmdId)
   {
     case CMDID_EVENTS:                                                         /* event notifications */
         /* parse notification */
         dn_ipmt_events_notif = (dn_ipmt_events_nt*)app_vars.notifBuf;

         switch (dn_ipmt_events_notif->state)
            {
               case MOTE_STATE_IDLE:
                    Flag_Check=Mote_Status;
                    delay_count=0;
                    awaiting_response=false;
                    dn_ipmt_cancelTx();                                         /* Incase of reset event */
                    txRun=0;  // NOTE: This was changed to 0 from 1... Don't know why you would want it to be 1
                    txPacketDone=1;
                    break;
               default:
                    /* nothing done here */
                    break;
            }

         switch (dn_ipmt_events_notif->events)
            {
               case JoinStarted:
                    delay_count=0;
                    break;
               case Operational:
                    delay_count=0;
                    break;
               case SvcChange:
                    awaiting_response=false;
                    delay_count=0;
                    Flag_Check=Get_Service_Info;

                    //if(MoteStatus!=MoreBW)
                    //{
                        //Flag_Check=Request_Service;
                    //}
                    //else
                    //{
                        //Flag_Check=Get_Service_Info;
                    //}
                    break;
               default:
                    /* nothing done here */
                    break;
            }

     case CMDID_TIMEINDICATION:                                                 /* Time Indication notifications */
          /* Time Information notifications are handled here */
          break;

     case CMDID_RECEIVE:                                                        /* Packet Received notifications */
          /* Packet Received notifications are handled here */
          /*Setting pointer to payload*/
          dn_ipmt_receive_notif = (dn_ipmt_receive_nt*)app_vars.notifBuf;
          /*Filling parameter arrays with received values*/
          for(int i=0; i<8; i++)
            {
              sampFrequencyArray[i] = dn_ipmt_receive_notif->payload[i];
              alarmArray[i]         = dn_ipmt_receive_notif->payload[i+8];
              axisArray[i]          = dn_ipmt_receive_notif->payload[i+16];
              numSampArray[i]       = dn_ipmt_receive_notif->payload[i+24];
              sleepDurArray[i]      = dn_ipmt_receive_notif->payload[i+32];
              cmdDescriptorArray[i] = dn_ipmt_receive_notif->payload[i+40]; 
            }

          cmdDescriptor  = (uint16_t)atol(cmdDescriptorArray);

          if (cmdDescriptor == 11)
          {
             // Manager Ready Signal
             mgrReady = true;
          }
          else if (cmdDescriptor == 22)
          {

             // Manager Ready Signal
             mgrReady = true;

             /*Convert char arrays to long int (32) format*/
             samp_frequency  = (uint32_t)atol(sampFrequencyArray);
             axis_info       = (uint8_t)atol(axisArray);
             adcNumSamples   = (uint32_t)atol(numSampArray);
             sleep_dur_s     = (uint32_t)atol(sleepDurArray);
             alarm           = (uint8_t)atol(alarmArray);

             DEBUG_PRINT(("adcNumSamples=%d\n", adcNumSamples));
             DEBUG_PRINT(("sleep_dur_s=%d\n", sleep_dur_s));
             DEBUG_PRINT(("axis info = %d\n", axis_info));
          }
          else if (cmdDescriptor == 33)
          {
              // Axis info only
             axis_info = (uint8_t)atol(axisArray);
             DEBUG_PRINT(("axis info = %d\n", axis_info));
          }
          else if (cmdDescriptor == 44)
          {
             alarm = (uint8_t)atol(alarmArray);
          }

          /* Toggle Red LED if alarm has been set, disable Green LED */
          if (alarm)
          {
              adi_gpio_SetHigh(ADI_GPIO_PORT1, ADI_GPIO_PIN_12);
              adi_gpio_OutputEnable(ADI_GPIO_PORT1, ADI_GPIO_PIN_12, false);
              adi_gpio_OutputEnable(ADI_GPIO_PORT1, ADI_GPIO_PIN_13, true);
              adi_gpio_SetLow(ADI_GPIO_PORT1, ADI_GPIO_PIN_13);

          }
          else // Enable green LED
          {
              adi_gpio_SetHigh(ADI_GPIO_PORT1, ADI_GPIO_PIN_13);
              adi_gpio_OutputEnable(ADI_GPIO_PORT1, ADI_GPIO_PIN_12, true);
          }
          break;

     case CMDID_MACRX:                                                          /* Mac Receive notifications */
          /* MacRx notifications are handled here */
          break;
     case CMDID_TXDONE:                                                         /* Tramission done notifications */
          /* Transmit Done notifications are handled here */
          //DEBUG_PRINT(("TX DONE\n"));
          txPacketDone = 1;
          break;
     case CMDID_ADVRECEIVED:                                                    /* Advertisement notifications */
          /* Received advertisement notifications are handled here */
          break;
     default:
          /* nothing to do */
          break;
   }
}


/*=========================== User Application ================================*/
/**
 * @brief    User application sits here.
 *
 * @param       void.
 *
 * @return  uint8_t*    Pointer to an array for transmission is returned .
 *
 * User has the liberty to edit this function to do application specific
 * tasks and send data to be transmitted.
 *
 *
 * @sa
 *
 *
 * @note
 *
 */

// Getter for sampling frequency, used to update FFT calculation in main_prog.c
uint32_t getSampFreq()
{
  return samp_frequency;
}

uint32_t getAdcNumSamples()
{
   return adcNumSamples;
}

// Getter for sleep duration, used in main_prog.c
uint32_t getSleepDur()
{
  return sleep_dur_s;
}

/* Calculate appropriate timeout for given sampling frequency */
int getSampTimeout(uint32_t samp_freq)
{
  uint32_t acquisition_time = 1000000/samp_freq; //Time needed for one FFT reading
  uint8_t timeouts = acquisition_time / SAMPLING_SCHEDULER_CLOCK_USEC; //Timeouts needed to count
  return timeouts;
}

/* Calculate the required time between samples in microseconds 
 * Program the timer with this value directly so that it just needs to trigger
 * once instead of counting how many times 10us elapses */
uint32_t getSampTime_us(uint32_t samp_freq)
{
  uint32_t  samp_time_us = 1000000/samp_freq; // Time between samples
  return samp_time_us;
}

uint8_t getSamptime(uint32_t ACLOCK, uint8_t resolution)
{
    uint8_t N = resolution; //Default 64
    samp_time = (uint8_t)((ACLOCK / (getSampFreq() * N))-14);
    return samp_time;
}

uint8_t getExtraBits()
{
    return (extra_bits_var - 12); //Number of bits oversampled
}

uint8_t getResolution()
{
    uint8_t n = getExtraBits();
    uint8_t res = (uint8_t)pow(2, 2*n);
    return res;
}


uint8_t getAxisInfo()
{
   return axis_info;
}

/**
 * @brief    Execute reply call back from API.
 *
 * @param [in] cmdId      command ID.
 *
 * @return  void.
 *
 * Executes reply call back function upon receving reply from API.
 *
 * @note    NA.
 */

void dn_ipmt_reply_cb(uint8_t cmdId)
{
   app_vars.replyCb();                                                          /* API callback  function */
}


/*=========================== A P I s ========================================*/

/*=========================== responseTimeout ================================*/
/**
 * @brief    Function executed when time our occurs.
 *
 * @param void.
 *
 * @return  void.
 *
 * Executes getMoteStatus function when time out occurs.
 *
 * @sa      dn_ipmt_cancelTx().
 * @sa      scheduleEvent().
 * @sa      api_getMoteStatus().
 *
 * @note   This function is only used when the application is written using RTC delays,
 *         the current application does not use this function .
 */

void api_response_timeout(void) {
   /* issue cancel command */
   dn_ipmt_cancelTx();

   /* Reset Flag values */
   MoteStatus=CheckingStatus;
   Flag_Check=Mote_Status;

   /* schedule status event */
   scheduleEvent(&api_getMoteStatus);
}


/*=========================== getMoteStatus ==================================*/
/**
 * @brief    Requests for the status of the Module.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Status of the module is obtained once this function executes.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_getParameter_moteStatus().
 *
 * @note    NA.
 */
void api_getMoteStatus(void)
{
   /* API callback */
   setCallback(api_getMoteStatus_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_getParameter_moteStatus(
      (dn_ipmt_getParameter_moteStatus_rpt*)(app_vars.replyBuf)
   );
}

/**
 * @brief   Reply  for api_getMoteStatus is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply to the getMoteStatus is processed here.
 *
 * @sa      cancelEvent().
 * @sa      scheduleEvent().
 * @sa      api_getMoteStatus().
 *
 * @note    NA.
 */
void api_getMoteStatus_reply(void)
{
   dn_ipmt_getParameter_moteStatus_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_getParameter_moteStatus_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->state)
   {
      case MOTE_STATE_IDLE:
           awaiting_response=false;
           delay_count=0;
           MoteStatus=Idle;
           Flag_Check=Open_Socket;
           break;
      case MOTE_STATE_OPERATIONAL:
           awaiting_response=false;
           delay_count=0;
           MoteStatus=Joined;
           Flag_Check=SendTo;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Mote_Status;
           scheduleEvent(&api_getMoteStatus);                                   /*Ask for status of the mote */
           break;
   }
}


/*=========================== openSocket ======================================*/
/**
 * @brief   Requests for Opening a Socket for data transfer.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Functions opens up a socket for data transfer,the protocol used here is UDP.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_openSocket().
 *
 * @note    NA.
 */
void api_openSocket(void)
{

   /* API callback */
   setCallback(api_openSocket_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_openSocket(
      0,                                              /* protocol */
      (dn_ipmt_openSocket_rpt*)(app_vars.replyBuf)    /* reply */
   );

 }

/**
 * @brief   Reply  for api_openSocket is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply contains the socket ID .
 *
 * @sa      cancelEvent().
 * @sa       scheduleEvent().
 * @sa       api_openSocket().
 *
 * @note    NA.
 */
void api_openSocket_reply(void)
{
   dn_ipmt_openSocket_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_openSocket_rpt*)app_vars.replyBuf;

      /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Bind_Socket;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Open_Socket;
           scheduleEvent(&api_openSocket);                                      /*Send Open socket request again */
           break;
   }

   /* store the socketID */
   app_vars.socketId = reply->socketId;
}


/*=========================== bindSocket ======================================*/
/**
 * @brief    Requests for the binding the socket opened to a port.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Binding the socket ID with the source port.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_bindSocket().
 *
 * @note    NA.
 */
void api_bindSocket(void)
{

   /* API callback */
   setCallback(api_bindSocket_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_bindSocket(
      app_vars.socketId,                              /* socketId */
      SRC_PORT,                                       /* port */
      (dn_ipmt_bindSocket_rpt*)(app_vars.replyBuf)    /* reply */
   );
}

/**
 * @brief    Reply  for api_bindSocket is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply just confirms that the socket ID was bound to the specified port.
 *
 * @sa      cancelEvent().
 * @sa       scheduleEvent().
 * @sa       api_bindSocket().
 *
 * @note    NA.
 */
void api_bindSocket_reply(void)
{
   dn_ipmt_bindSocket_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_bindSocket_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           MoteStatus=Setting_Param;
           Flag_Check=Set_NetID;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Bind_Socket;
           scheduleEvent(&api_bindSocket);                                      /*Send bind socket request again */
           break;
   }
}


/*=========================== setNetworkID ===================================*/
/**
 * @brief    Sets the network ID.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Status of the module is obtained once this function executes.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_setParameter_networkId().
 *
 * @note    NA.
 */
void api_setNetworkID(void)
{

   /* API callback */
   setCallback(api_setNetworkID_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_setParameter_networkId(MANAGER_NETID,
      (dn_ipmt_setParameter_networkId_rpt*)(app_vars.replyBuf)
   );
}

/**
 * @brief   Reply  for api_setNetworkID is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 *
 *
 * @sa      cancelEvent().
 * @sa      scheduleEvent().
 * @sa      api_setNetworkID().
 *
 * @note    NA.
 */
void api_setNetworkID_reply(void)
{
   dn_ipmt_getParameter_networkId_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_getParameter_networkId_rpt*)app_vars.replyBuf;


   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Get_NetID;
           MoteStatus = Getting_Param;
           break;

      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Set_NetID;
           scheduleEvent(&api_setNetworkID);
           break;
   }
}
/*=========================== getNetworkID ===================================*/
/**
 * @brief    Requests for the network ID.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Status of the module is obtained once this function executes.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_getParameter_networkId().
 *
 * @note    NA.
 */
void api_getNetworkID(void)
{

   /* API callback */
   setCallback(api_getNetworkID_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_getParameter_networkId(
      (dn_ipmt_getParameter_networkId_rpt*)(app_vars.replyBuf)
   );
}

/**
 * @brief   Reply  for api_getNetworkID is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply to the getMoteStatus is processed here.
 *
 * @sa      cancelEvent().
 * @sa      scheduleEvent().
 * @sa      api_getNetworkID().
 *
 * @note    NA.
 */
void api_getNetworkID_reply(void)
{
   dn_ipmt_getParameter_networkId_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_getParameter_networkId_rpt*)app_vars.replyBuf;

   /*Save the network ID parameter*/
   NET_ID = reply->networkId;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Set_JoinDC;
           MoteStatus = Setting_Param;
           break;

      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Get_NetID;
           scheduleEvent(&api_getNetworkID);
           break;
   }
}

/*=========================== setJoinDutyCycle ===================================*/
/**
 * @brief    Sets the Join Duty Cycle
 *
 * @param   JDC, join duty cycle 0=0.2%, 255=99.8%
 *
 * @return  void.
 *
 * Status of the module is obtained once this function executes.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_setParameter_joinDutyCycle().
 *
 * @note    NA.
 */
void api_setJoinDutyCycle(void)
{

   /* API callback */
   setCallback(api_setJoinDutyCycle_reply);

   awaiting_response=true;
   delay_count=0;

   /* issue function */
   dn_ipmt_setParameter_joinDutyCycle(join_duty,(dn_ipmt_setParameter_joinDutyCycle_rpt*)app_vars.replyBuf);
}

/**
 * @brief   Reply  for setJoinDutyCycle is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply  for setJoinDutyCycle is processed here.
 * @sa      cancelEvent().
 * @sa      scheduleEvent().
 * @sa     api_setJoinDutyCycle().
 *
 * @note    NA.
 */
void api_setJoinDutyCycle_reply(void)
{
   dn_ipmt_setParameter_joinDutyCycle_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_setParameter_joinDutyCycle_rpt*)app_vars.replyBuf;


   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Join;
           break;

      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Set_JoinDC;
           scheduleEvent(&api_setJoinDutyCycle);
           break;
   }
}

/*=========================== join ============================================*/
/**
 * @brief    Network Join command is issued here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * The module is requested to join the network.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_join().
 *
 * @note    Currently default paramenters of the SmartMesh IP network is used.
 */

void api_join(void)
{

   /* API callback */
   setCallback(api_join_reply);

   awaiting_response=true;
   delay_count = 0;

   /* issue function */
   dn_ipmt_join(
      (dn_ipmt_join_rpt*)(app_vars.replyBuf)     /* reply */
   );

}

/**
 * @brief    Reply to the api_join is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply to the join request is handled.
 *
 * @sa      cancelEvent().
 * @sa       scheduleEvent().
 * @sa       api_join().
 *
 * @note    NA.
 */
void api_join_reply(void)
{
   dn_ipmt_join_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_join_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           delay_count=0;
           MoteStatus=Joined;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Join;
           scheduleEvent(&api_join);                                            /* Send Join request again */
           break;
   }
}


/*=========================== requestService =================================*/
/**
 * @brief    Network Request Service command is issued here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * The module is requested to join the network.
 *
 * @sa      setCallback().
 * @sa      dn_ipmt_xxxx().
 *
 * @note    Currently default paramenters of the SmartMesh IP network is used.
 */
void api_requestService(void)
{

   /* API callback */
   setCallback(api_requestService_reply);

   awaiting_response=true;
   delay_count = 0;

   /* issue function */
   dn_ipmt_requestService(
       MANAGER_ADDRESS,                                 /*destination address*/
       SERVICE_TYPE_BW,                                 /*bandwidth service only*/
       ms_per_packet,                                  /*ms between packets requested*/
       (dn_ipmt_requestService_rpt*)app_vars.replyBuf   /*buffer for saving reply*/
   );

}

/**
 * @brief    Reply to the api_requestService is processed here.
 *
 * @param   void.
 *
 * @return  void.
 *
 * Reply to the request service is handled.
 *
 * @sa      cancelEvent().
 * @sa       scheduleEvent().
 * @sa       api_xxxx().
 *
 * @note    NA.
 */
void api_requestService_reply(void)
{
   dn_ipmt_requestService_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_requestService_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           delay_count=0;
           MoteStatus=MoreBW;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           /* Send Request Service request again */
           Flag_Check=Request_Service;
           scheduleEvent(&api_requestService);
           break;
   }
}

/* Identify packet header (FF) and axis (90) */
#define MAX_PAYLOAD_SIZE 90
#define X_AXIS 0xFF90
#define Y_AXIS 0xFFA0
#define Z_AXIS 0xFFC0

static uint8_t* pByteBuf;
static uint32_t numBytesLeft;


/*=========================== startTx ========================================*/
/**
 * @brief    This function begins the transmission of the ADC data buffers.
 *
 * @param   void.
 *
 * @return  void.
 *
 * startTx manages which axis (X,Y or Z) will transmitted first based on axis_info
 * variable, and begins transmission by flagging txRun.
 *
 * @note    Function also sets numBytesLeft, a variable which keeps track of
 * bytes left for sending in a buffer.
 */

void startTx(bool include_hdr, axis_t axis_tx)
{ 
    
    DEBUG_PRINT(("Starting TX\n"));
    
    // Check if header needs to be included. When using flash, the data will be
    // TXd 1 page at a time... The header should only be included 
    // at the start of the 1st page of data
    if (!ext_flash_needed)
    {
        // Give the first four bits in the 2nd byte in the header the axis info
        // (The first byte in the header is all ones)
        adcDataX[0] = X_AXIS;
        adcDataY[0] = Y_AXIS;
        adcDataZ[0] = Z_AXIS;

        uint16_t version_bits = int_version;

        // Give the last 4 bits in the 2nd byte of the header the version info
        adcDataX[0] |= version_bits;
        adcDataY[0] |= version_bits;
        adcDataZ[0] |= version_bits;
    
        // Select where pointer should start from based on which axes are enabled

        switch(axis_info)
        {
            case XYZ:
            case XY:
            case XZ:
            case X:
            case 0:
                pByteBuf = (uint8_t*) adcDataX;
                break;
            case YZ:
            case Y:
                pByteBuf = (uint8_t*) adcDataY;
                break;
            case Z:
                pByteBuf = (uint8_t*) adcDataZ;
                break;
            default:
                pByteBuf = (uint8_t*) adcDataX;
        }
    }
    else
    {
        // If ext flash is needed then the adcDataX array shall 
        // be used for all tranmissions of data
        pByteBuf = (uint8_t*) adcDataX;

        // Check if header needs to be included. When using flash, the data will be
        // TXd 1 page at a time... The header should only be included 
        // at the start of the 1st page of data
        if (include_hdr)
        {
          // Give the first four bits in the 2nd byte in the header the axis info
          // (The first byte in the header is all ones)
          switch (axis_tx)
          {
             case x_active : adcDataX[0] = X_AXIS;
                             break;
             case y_active : adcDataX[0] = Y_AXIS; 
                             break;
             case z_active : adcDataX[0] = Z_AXIS; 
                             break;
          }

          uint16_t version_bits = int_version;

          // Give the last 4 bits in the 2nd byte of the header the version info
          adcDataX[0] |= version_bits;
       }
       else
       {
           // Start sending out from the second location of adcDataX (starting at 3rd byte)
           pByteBuf += 2; 
           ADC_DATA_SIZE -= 4;
       }
    }

    //DEBUG_PRINT(("%x%x\n", *pByteBuf, *(pByteBuf+1)));
    // ADC_DATA_SIZE will be updated after each time the data is fetched from flash
    numBytesLeft = ADC_DATA_SIZE;
    DEBUG_PRINT(("Sending %dB, Starting @ addr %x\n", numBytesLeft, pByteBuf));
    DEBUG_PRINT(("Final Addr = %x\n", pByteBuf+ADC_DATA_SIZE));
    txRun    = 1;
    finalAck = 0;
}

int txRunning(void)
{
    return txRun;
}

int gotFinalAck(void)
{
    return finalAck;
}


/*=========================== sendTo =========================================*/
/**
 * @brief    This function handles the transmission of data packets to a ipv6 address.
 *
 * @param   void.
 *
 * @return  void.
 *
 * api_sendTo function handles the transmission of data packets to a ipv6 address.
 * here the address is of the manager.
 *
 * @sa      setCallback().
 * @sa      api_sendTo().
 * @sa      memcpy().
 *
 * @note    Data is sent to the manager.Destination and source ports are mentioned in the api_ports section
 *          of the #defines and the packet ID submitted does not require transmission done acknowledgement.
 */
uint32_t packets_sent = 0;
uint32_t packets_ackd = 0;
void api_sendTo(void)
{
    uint8_t numBytes;

    /* Setting txRun and txPacketDone from another function will cause this
     * function to send data via the SmartMesh
     * txPacketDone is set to 1 when CMDID_TXDONE */
    if (!txRun || !txPacketDone)
        return;


    if (numBytesLeft > MAX_PAYLOAD_SIZE)
        numBytes = MAX_PAYLOAD_SIZE;
    else
        numBytes = (uint8_t) numBytesLeft;

    setCallback(api_sendTo_reply);
    awaiting_response=true;
    txPacketDone = 0;

    dn_ipmt_sendTo(
            app_vars.socketId,                                  /* socketId */
            (uint8_t*) ipv6Addr_manager,                        /* destIP */
            DST_PORT,                                           /* destPort */
            SERVICE_TYPE_BW,                                    /* serviceType */
            MED_PRIORITY,                                       /* priority */
            0xBEEF,                                             /* packetId */
            pByteBuf,                                           /* payload */
            numBytes,                                           /* payloadLen */
            (dn_ipmt_sendTo_rpt*)(app_vars.replyBuf)            /* reply */
    );

    // Toggle green LED
    adi_gpio_Toggle(ADI_GPIO_PORT1, ADI_GPIO_PIN_12);

    pByteBuf += numBytes;
    numBytesLeft -= numBytes;

    /* Decide if all data has been transmitted and move pointer if necessary*/
    if (!ext_flash_needed)
    {
        if (numBytesLeft <= 0)
        {
            // If all of a particular axis' data has been sent
            numBytesLeft = ADC_DATA_SIZE;

            if(pByteBuf == (uint8_t*)adcDataX+ADC_DATA_SIZE)
            {
                // X data transmission finished
                switch(axis_info)
                {
                    // Y data is next
                    case XYZ:
                    case XY:
                        /* Move pointer from to start of Y data*/
                        pByteBuf = (uint8_t*)adcDataY;
                        break;
                        
                    // Z data is next
                    case XZ:
                        /* Move pointer to start of Z data*/
                        pByteBuf = (uint8_t*)adcDataZ;
                        break;

                    // All data sent
                    case X:
                        txRun = 0;
                        packets_sent = 0;
                        break;
                }
            }
            else if(pByteBuf == (uint8_t*)adcDataY+ADC_DATA_SIZE)
            {
                // Y data transmission finished
                switch(axis_info)
                {
                    // Z data is next
                    case XYZ:
                    case YZ:
                        /* Move pointer Z data*/
                        pByteBuf = (uint8_t*)adcDataZ;
                        break;
                        
                    // All data sent
                    case XY:
                    case Y:
                        txRun = 0;
                        packets_sent = 0;
                        break;
                }
            }
            else if(pByteBuf == (uint8_t*)adcDataZ+ADC_DATA_SIZE)
            {
                // All data sent
                txRun = 0;
                packets_sent = 0;
            }
        } 
        else
        {
            packets_sent++;
        }
    }
    else
    {
        // Flash is needed... adcDataX is the only array used for sending data in this case
        if(pByteBuf >= (uint8_t*)adcDataX+ADC_DATA_SIZE)
        {
            txRun = 0;
            packets_sent = 0;
        }
        else
        {
            packets_sent++;
        }

    }
    //DEBUG_PRINT(("Data Sent. Num Bytes Left: %d\n", numBytesLeft));
}

/**
 * @brief    Reply to sendTo request is handled here .
 *
 * @param   void.
 *
 * @return  void.
 *
 * sendTo reply processed here.
 *
 * @sa      cancelEvent().
 * @sa      scheduleEvent().
 * @sa      api_sendTo().
 *
 * @note    NA.
 */
void api_sendTo_reply(void)
{
   dn_ipmt_sendTo_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   if (!txRun)
   {
     /* Get the final acknowledge before allowing main FSM to move out of TX */
     finalAck = 1;
     packets_ackd = 0;
   }

   /* parse reply */
   reply = (dn_ipmt_sendTo_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           packets_ackd++;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           break;
   }
}

void api_disconnect(void)
{
    /* API callback */
    setCallback(api_disconnect_reply);
    awaiting_response=true;

    /* issue function */
    dn_ipmt_disconnect(
            (dn_ipmt_disconnect_rpt*)(app_vars.replyBuf)  /* reply */
    );

}

void api_disconnect_reply(void)
{
   dn_ipmt_disconnect_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_disconnect_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           MoteStatus=Disconnected;
           Flag_Check=Disconnecting;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Disconnecting;
           scheduleEvent(&api_disconnect);  /* Send Disconnect request again */
           break;
   }
}

void api_lowPowerSleep(void)
{
    /* API callback */
    setCallback(api_lowPowerSleep_reply);
    awaiting_response=true;

    /* issue function */
    dn_ipmt_lowPowerSleep(
            (dn_ipmt_lowPowerSleep_rpt*)(app_vars.replyBuf)  /* reply */
    );
}


void api_lowPowerSleep_reply(void)
{
   dn_ipmt_lowPowerSleep_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_lowPowerSleep_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           MoteStatus=Deep_Sleep;
           Flag_Check=Going_To_Sleep;
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           Flag_Check=Going_To_Sleep;
           scheduleEvent(&api_lowPowerSleep);  /* Send lowPowerSleep request again */
           break;
   }
}


void api_getServiceInfo(void)
{
    /* API callback */
    setCallback(api_getServiceInfo_reply);
    awaiting_response=true;

    /* issue function */
    dn_ipmt_getServiceInfo(
       MANAGER_ADDRESS,                                 /*destination address*/
       SERVICE_TYPE_BW,                                 /*bandwidth service only*/
      (dn_ipmt_getServiceInfo_rpt*)(app_vars.replyBuf)  /* reply */
   );
}


void api_getServiceInfo_reply(void)
{
   dn_ipmt_getServiceInfo_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_getServiceInfo_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           inter_packet_interval=reply->value;
           DEBUG_PRINT(("Reported IPI: %d\n", inter_packet_interval));
           if (inter_packet_interval > ms_per_packet + 10)
           {
               Flag_Check=Request_Service;
           }
           else 
           {
              Flag_Check=SendTo;
           }
           break;
      default:
           awaiting_response=false;
           delay_count=0;
           scheduleEvent(&api_getServiceInfo);  /* Send api_getServiceInfo request again */
           inter_packet_interval=0;
           break;
   }
}

bool getMgrReady()
{
    return mgrReady;
}

void clearMgrReady()
{
    mgrReady = false;
}

void sendMgrReady()
{
    //DEBUG_PRINT(("Send Rdy\n"));

    uint8_t numBytes;
    numBytes = 4;
    setCallback(api_readySignal_reply);
    awaiting_response=true;
    txPacketDone = 0;

    uint8_t ackMsg [4];
    ackMsg[0] = 0xAA;
    ackMsg[1] = 0xBB;
    ackMsg[2] = 0xCC;
    ackMsg[3] = 0xDD;

    uint8_t *ackPtr;
    ackPtr = &ackMsg[0];

    dn_ipmt_sendTo(
            app_vars.socketId,                                  /* socketId */
            (uint8_t*) ipv6Addr_manager,                        /* destIP */
            DST_PORT,                                           /* destPort */
            SERVICE_TYPE_BW,                                    /* serviceType */
            MED_PRIORITY,                                       /* priority */
            0xBEEF,                                             /* packetId */
            ackPtr,                                             /* payload */
            numBytes,                                           /* payloadLen */
            (dn_ipmt_sendTo_rpt*)(app_vars.replyBuf)            /* reply */
    );
}

void api_readySignal_reply(void)
{
   dn_ipmt_sendTo_rpt* reply;

   /* cancel timeout */
   cancelEvent();

   /* parse reply */
   reply = (dn_ipmt_sendTo_rpt*)app_vars.replyBuf;

   /* choose next step */
   switch (reply->RC)
   {
      case RC_OK:
           awaiting_response=false;
           delay_count=0;
           break;
      default:
           awaiting_response=false;
           scheduleEvent(&sendMgrReady);  /* Send api_getServiceInfo request again */
           delay_count=0;
           break;
   }
}


void sendMsg(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    uint8_t numBytes;
    numBytes = 4;
    setCallback(api_readySignal_reply);
    awaiting_response=true;
    txPacketDone = 0;

    uint8_t ackMsg [4];
    ackMsg[0] = a;
    ackMsg[1] = b;
    ackMsg[2] = c;
    ackMsg[3] = d;

    uint8_t *ackPtr;
    ackPtr = &ackMsg[0];

    dn_ipmt_sendTo(
            app_vars.socketId,                                  /* socketId */
            (uint8_t*) ipv6Addr_manager,                        /* destIP */
            DST_PORT,                                           /* destPort */
            SERVICE_TYPE_BW,                                    /* serviceType */
            MED_PRIORITY,                                       /* priority */
            0xBEEF,                                             /* packetId */
            ackPtr,                                             /* payload */
            numBytes,                                           /* payloadLen */
            (dn_ipmt_sendTo_rpt*)(app_vars.replyBuf)            /* reply */
    );
}

void sendStartingTx()
{
    sendMsg(0xD, 0xD, 0x0, 0x0);
}

void sendFinishedTx()
{
    sendMsg(0xD, 0xE, 0x0, 0x0);
}

void sendAcqMsg()
{
    sendMsg(0xB, 0xC, 0x0, 0x0);
}

void sendNewParamMsg()
{
    sendMsg(0xA, 0xB, 0x0, 0x0);
}

void sendCalcMsg()
{
    sendMsg(0xA, 0xA, 0x0, 0x0);
}

void sendWakeUpMsg()
{
    sendMsg(0xE, 0xF, 0x0, 0x0);
}


