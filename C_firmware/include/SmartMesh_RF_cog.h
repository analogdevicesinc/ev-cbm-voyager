/*
 *****************************************************************************
 * @file:    SmartMesh_RF_cog.h TODO: Rename with lower case only
 * @brief:   Header file for SmartMesh_RF_cog.c
 *****************************************************************************

*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated Analog Devices
License Agreement.
*********************************************************************************
*/

#ifndef SMART_MESH__
#define SMART_MESH__

/*=========================== Includes ========================================*/
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <drivers/i2c/adi_i2c.h>
#include <adi_processor.h>
#include <drivers/pwr/adi_pwr.h>
#include <drivers/gpio/adi_gpio.h>
//#include <common.h>

#include "ADC_channel_read.h"
#include "dn_ipmt.h"
#include "dn_uart.h"

/*=========================== Defines =========================================*/

/* mote state */
#define MOTE_STATE_IDLE           0x01
#define MOTE_STATE_SEARCHING      0x02
#define MOTE_STATE_NEGOCIATING    0x03
#define MOTE_STATE_CONNECTED      0x04
#define MOTE_STATE_OPERATIONAL    0x05

/* Events */
#define JoinStarted               256
#define Operational               32
#define SvcChange                 128

/* service types */
#define SERVICE_TYPE_BW           (uint8_t)0x00

/*Data Packet Priority*/
#define HIGH_PRIORITY             0x02
#define MED_PRIORITY              0x01
#define LOW_PRIORITY              0x00

/* number of milli-seconds between data packets requested*/
#define DATA_PERIOD_MS            500

/* api_ports */
#define SRC_PORT                  0xf0b8
#define DST_PORT                  0xf0b9

/*Manager address*/
#define MANAGER_ADDRESS           (uint16_t)0xfffe

/*Manager NET_ID*/
#define MANAGER_NETID             (uint16_t)2425

/*Service types*/
#define BANDWIDTH_SV               (uint16_t)0x0

/*Transmit payloads for different types of data*/
#define TX_PAYLOAD_RAW            90u
#define TX_PAYLOAD_FFT            90u

/* dummy data */
#define dummy_data                0x08

/*Delay global variable */
#define delay_count_ten_msec      260000
#define delay_count_one_sec       26000000
#define delay_count_two_sec       52000000
#define delay_count_ten_sec       260000000
#define delay_count_dara          2600000

/* Receive Correct */
#define RC_OK                     0x00

/* Sensor data acquisition */
#define Sensor_data_enable        0

#define XYZ 111
#define XY  110
#define XZ  101
#define X   100
#define YZ  11
#define Y   10
#define Z   1

/*=========================== Typedef =========================================*/

typedef void (*timer_callback)(void);
typedef void (*reply_callback)(void);

typedef enum {
  Disconnecting,
  Going_To_Sleep,
  Boot_Status,                                            /* Flag_Check value on booting */
  Mote_Status,                                            /* Flag_Check value for getting mote status */
  Open_Socket,                                            /* Flag_Check value for opening a socket for communication */
  Bind_Socket,                                            /* Flag_Check value for binding the socket with a source por t */
  Set_NetID,                                              /* Flag Check for getting the network ID*/
  Get_NetID,                                              /* Flag Check for setting the network ID*/
  Set_JoinDC,                                              /* Flag Check for setting the join duty cycle*/
  Join,                                                   /* Flag_Check value for joining a network */
  Request_Service,                                        /* Flag Check value for requesting bandwidth*/
  Get_Service_Info,
  SendTo                                                  /* Flag_Check value for sending data to a IPv6 address */
} State_Check;

typedef enum {
  Disconnected,
  Deep_Sleep,
  Booting,
  CheckingStatus,
  Idle,
  Getting_Param,
  Setting_Param,
  Joining,
  MoreBW,
  Joined
} Mote_Status_t;


typedef struct {
   /* event */
   timer_callback   eventCb;
   /* reply */
   reply_callback   replyCb;
   /* app */
   uint8_t              secUntilTx;
   uint8_t              direction;
   /* api */
   uint8_t              socketId;                          /* ID of the mote's UDP socket */
   uint8_t              replyBuf[MAX_FRAME_LENGTH];        /* holds notifications from ipmt */
   uint8_t              notifBuf[MAX_FRAME_LENGTH];        /* notifications buffer internal to ipmt */
} app_vars_t;

/*=========================== Prototypes =======================================*/
/* ipmt */
void dn_ipmt_notif_cb(uint8_t cmdId, uint8_t subCmdId);
void dn_ipmt_reply_cb(uint8_t cmdId);

void buffer_handle(void);

/* api */

void api_response_timeout(void);
void api_getMoteStatus(void);
void api_getMoteStatus_reply(void);
void api_openSocket(void);
void api_openSocket_reply(void);
void api_bindSocket(void);
void api_bindSocket_reply(void);
void api_setNetworkID(void);
void api_setNetworkID_reply(void);
void api_getNetworkID(void);
void api_getNetworkID_reply(void);
void api_setJoinDutyCycle(void);
void api_setJoinDutyCycle_reply(void);
void api_join(void);
void api_join_reply(void);
void api_requestService(void);
void api_requestService_reply(void);
void api_sendTo(void);
void api_sendTo_reply(void);
void api_disconnect(void);
void api_disconnect_reply(void);
void api_lowPowerSleep(void);
void api_lowPowerSleep_reply(void);
void api_getServiceInfo(void);
void api_getServiceInfo_reply(void);

void scheduleEvent(timer_callback cb);
void startTx(bool, axis_t);
int txRunning(void);
int gotFinalAck(void);
uint32_t getAdcNumSamples(void);
uint32_t getSampFreq(void);
uint32_t getSleepDur(void);
int getSampTimeout(uint32_t);
uint8_t getSamptime(uint32_t, uint8_t);
uint32_t getSampTime_us(uint32_t);
uint8_t getExtraBits(void);
uint8_t getResolution(void);
uint8_t getAxisInfo(void);

bool getMgrReady(void);
void clearMgrReady(void);
void sendMgrReady(void);
void api_readySignal_reply(void);

void sendMsg(uint8_t, uint8_t, uint8_t, uint8_t);
void sendStartingTx(void);
void sendFinishedTx(void);
void sendAcqMsg(void);
void sendNewParamMsg(void);
void sendCalcMsg(void);
void sendWakeUpMsg(void);


/*=========================== Variables =======================================*/

extern uint32_t inter_packet_interval;

#endif // SMART_MESH__
