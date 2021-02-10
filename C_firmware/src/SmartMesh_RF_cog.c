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


/*=======================  D E F I N E S   ===================================*/


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
static uint32_t              samp_frequency   = 510;
static uint16_t              extra_bits_var   = 12;
static uint8_t               alarm            = 0;
static uint8_t               samp_time;
static uint8_t               axis_info        = 111;
static int txRun = 0;

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
                    txRun=1;
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
                    if(MoteStatus!=MoreBW)
                    {
                        Flag_Check=Request_Service;
                    }
                    else
                    {
                        Flag_Check=SendTo;
                    }
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
            }

          /*Convert char arrays to long int (32) format*/
          samp_frequency  = (uint32_t)atol(sampFrequencyArray);
          alarm           = (uint8_t)atol(alarmArray);
          axis_info       = (uint8_t)atol(axisArray);
          
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


/* Calculate appropriate timeout for given sampling frequency */ 
int getSampTimeout(uint32_t samp_freq)
{
  uint32_t acquisition_time = 1000000/samp_freq; //Time needed for one FFT reading
  uint8_t timeouts = acquisition_time / SAMPLING_SCHEDULER_CLOCK_USEC; //Timeouts needed to count 
  return timeouts; 
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
       MANAGER_ADDRESS,                                 /*Destination address*/
       SERVICE_TYPE_BW,                                 /*Bandwidth service only*/
       ms_per_packet,                                  /*Ms between packets requested*/
       (dn_ipmt_requestService_rpt*)app_vars.replyBuf   /*Buffer for saving reply*/
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

#define XYZ 111
#define XY  110
#define XZ  101
#define X   100
#define YZ  11
#define Y   10
#define Z   1

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

void startTx(void)
{       
    adcDataX[0] = X_AXIS;
    adcDataY[0] = Y_AXIS;
    adcDataZ[0] = Z_AXIS;
    
    uint16_t version_bits = int_version;
    
    adcDataX[0] |= version_bits;
    adcDataY[0] |= version_bits;
    adcDataZ[0] |= version_bits;
    
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
    
    numBytesLeft = ADC_DATA_SIZE; 
    txRun = 1;
}

int txRunning(void)
{
    return txRun;
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

void api_sendTo(void)
{
    uint8_t numBytes;

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

    adi_gpio_Toggle(ADI_GPIO_PORT1, ADI_GPIO_PIN_12);

    pByteBuf += numBytes;
    numBytesLeft -= numBytes;

    if (numBytesLeft <= 0)
    {
        numBytesLeft = ADC_DATA_SIZE;
        
        switch(axis_info)
        {  
        case XYZ:
        case YZ:
            if(pByteBuf == (uint8_t*)adcDataZ+ADC_DATA_SIZE)
            {  
                txRun = 0;
            }
            break;
        case XY:
            if(pByteBuf == (uint8_t*)adcDataY+ADC_DATA_SIZE)
            {  
                txRun = 0;
            }
            break;
        case XZ:
            if(pByteBuf == (uint8_t*)adcDataY)
            {
                pByteBuf = (uint8_t*)adcDataZ;
            }
            else
            {  
                txRun = 0;
            }
            break;
        case X:
        case Y:
        case Z:
        case 0:
            txRun = 0;
            break;
        }
    }  
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
           delay_count=0;
           break;
   }
}