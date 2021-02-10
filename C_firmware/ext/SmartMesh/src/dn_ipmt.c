/*
Copyright (c) 2015, Dust Networks. All rights reserved.

C library to connect to a SmartMesh IP Mote.

\license See attached DN_LICENSE.txt.
*/

#include "dn_ipmt.h"
#include "dn_lock.h"
#include "dn_serial_mt.h"

//=========================== variables =======================================

typedef struct {
   // sending requests
   uint8_t              outputBuf[MAX_FRAME_LENGTH];
   bool                 busyTx;
   uint8_t              cmdId;
   uint8_t              paramId;
   // receiving replies
   dn_ipmt_reply_cbt    replyCb;
   uint8_t*             replyContents;
   // receiving notifications
   dn_ipmt_notif_cbt    notifCb;
   uint8_t*             notifBuf;
   uint8_t              notifBufLen;
} dn_ipmt_vars_t;

dn_ipmt_vars_t dn_ipmt_vars;

//=========================== prototypes ======================================

// API
void dn_ipmt_setParameter_macAddress_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_joinKey_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_networkId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_txPower_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_joinDutyCycle_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_eventMask_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_OTAPLockout_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_routingMode_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_powerSrcInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_advKey_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_setParameter_autoJoin_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_macAddress_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_networkId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_txPower_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_joinDutyCycle_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_eventMask_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_moteInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_netInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_moteStatus_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_time_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_charge_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_testRadioRxStats_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_OTAPLockout_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_moteId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_ipv6Address_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_routingMode_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_appInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_powerSrcInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getParameter_autoJoin_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_join_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_disconnect_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_reset_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_lowPowerSleep_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_testRadioRx_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_clearNV_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_requestService_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_getServiceInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_openSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_closeSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_bindSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_sendTo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_search_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_testRadioTxExt_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_zeroize_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);
void dn_ipmt_socketInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);

// serial RX
void dn_ipmt_rxSerialRequest(uint8_t cmdId, uint8_t flags, uint8_t* payload, uint8_t len);

//=========================== public ==========================================

//========== admin

/**
\brief Setting up the instance.
*/
void dn_ipmt_init(dn_ipmt_notif_cbt notifCb, uint8_t* notifBuf, uint8_t notifBufLen, dn_ipmt_reply_cbt replyCb) {
   
   // reset local variables
   memset(&dn_ipmt_vars,0,sizeof(dn_ipmt_vars));
   
   // store params
   dn_ipmt_vars.notifCb         = notifCb;
   dn_ipmt_vars.notifBuf        = notifBuf;
   dn_ipmt_vars.notifBufLen     = notifBufLen;
   dn_ipmt_vars.replyCb         = replyCb;
   
   // initialize the serial connection
   dn_serial_mt_init(dn_ipmt_rxSerialRequest);
}

void dn_ipmt_cancelTx() {
   
   // lock the module
   dn_lock();
   
   dn_ipmt_vars.busyTx=FALSE;
   
   // unlock the module
   dn_unlock();
}



//========== API

//===== setParameter_macAddress

/**
This command allows user to overwrite the manufacturer-assigned MAC address of 
the mote. The new value takes effect after the mote resets. 
*/
dn_err_t dn_ipmt_setParameter_macAddress(uint8_t* macAddress, dn_ipmt_setParameter_macAddress_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_MACADDRESS;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_MACADDRESS;
   memcpy(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_MACADDRESS_REQ_OFFS_MACADDRESS],macAddress,8);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_MACADDRESS_REQ_LEN,                       // length
      dn_ipmt_setParameter_macAddress_reply                     // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_macAddress_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_macAddress_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_MACADDRESS_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_macAddress_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_joinKey

/**
The setParameter<joinKey> command may be used to set the join key in mote's 
persistent storage. Join keys are used by motes to establish secure connection 
with the network. The join key is used at next join.

Reading the joinKey parameter is prohibited for security reasons. 
*/
dn_err_t dn_ipmt_setParameter_joinKey(uint8_t* joinKey, dn_ipmt_setParameter_joinKey_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_JOINKEY;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_JOINKEY;
   memcpy(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_JOINKEY_REQ_OFFS_JOINKEY],joinKey,16);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_JOINKEY_REQ_LEN,                          // length
      dn_ipmt_setParameter_joinKey_reply                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_joinKey_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_joinKey_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_JOINKEY_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_joinKey_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_networkId

/**
This command may be used to set the Network ID of the mote. This setting is 
persistent and is used on next join attempt.

As of version 1.4.x, a network ID of 0xFFFF can be used to indicate that the 
mote should join the first network heard.

0xFFFF is never used over the air as a valid network ID - you should not set 
the Manager's network ID to 0xFFFF. 
*/
dn_err_t dn_ipmt_setParameter_networkId(uint16_t networkId, dn_ipmt_setParameter_networkId_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_NETWORKID;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_NETWORKID;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_NETWORKID_REQ_OFFS_NETWORKID],networkId);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_NETWORKID_REQ_LEN,                        // length
      dn_ipmt_setParameter_networkId_reply                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_networkId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_networkId_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_NETWORKID_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_networkId_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_txPower

/**
The setParameter<txPower> command sets the mote output power. This setting is 
persistent. The command may be issued at any time and takes effect on next 
transmission. Refer to product datasheets for supported RF output power values. 
For example, if the mote has a typical RF output power of +8 dBm when the power 
amplifier (PA) is enabled, then set the txPower parameter to 8 to enable the 
PA. Similarly, if the mote has a typical RF output power of 0 dBm when the PA 
is disabled, then set the txPower parameter to 0 to turn off the PA. 
*/
dn_err_t dn_ipmt_setParameter_txPower(int8_t txPower, dn_ipmt_setParameter_txPower_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_TXPOWER;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_TXPOWER;
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_TXPOWER_REQ_OFFS_TXPOWER] = (int8_t)txPower;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_TXPOWER_REQ_LEN,                          // length
      dn_ipmt_setParameter_txPower_reply                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_txPower_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_txPower_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_TXPOWER_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_txPower_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_joinDutyCycle

/**
The setParameter<joinDutyCycle> command allows the microprocessor to control 
the ratio of active listen time to doze time (a low-power radio state) during 
the period when the mote is searching for the network. If you desire a faster 
join time at the risk of higher power consumption, use the 
setParameter<joinDutyCycle> command to increase the join duty cycle up to 100%. 
This setting is persistent and takes effect immediately if the device is 
searching for network. 
*/
dn_err_t dn_ipmt_setParameter_joinDutyCycle(uint8_t dutyCycle, dn_ipmt_setParameter_joinDutyCycle_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_JOINDUTYCYCLE;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_JOINDUTYCYCLE;
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_JOINDUTYCYCLE_REQ_OFFS_DUTYCYCLE] = dutyCycle;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_JOINDUTYCYCLE_REQ_LEN,                    // length
      dn_ipmt_setParameter_joinDutyCycle_reply                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_joinDutyCycle_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_joinDutyCycle_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_JOINDUTYCYCLE_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_joinDutyCycle_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_eventMask

/**
The setParameter<eventMask> command allows the microprocessor to selectively 
subscribe to event notifications. The default value of eventMask at mote reset 
is all 1s - all events are enabled. This setting is not persistent.

New event type may be added in future revisions of mote software. It is 
recommended that the client code only subscribe to known events and gracefully 
ignore all unknown events. 
*/
dn_err_t dn_ipmt_setParameter_eventMask(uint32_t eventMask, dn_ipmt_setParameter_eventMask_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_EVENTMASK;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_EVENTMASK;
   dn_write_uint32_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_EVENTMASK_REQ_OFFS_EVENTMASK],eventMask);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_EVENTMASK_REQ_LEN,                        // length
      dn_ipmt_setParameter_eventMask_reply                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_eventMask_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_eventMask_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_EVENTMASK_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_eventMask_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_OTAPLockout

/**
This command allows the microprocessor to control whether Over-The-Air 
Programming (OTAP) of motes is allowed. This setting is persistent and takes 
effect immediately. 
*/
dn_err_t dn_ipmt_setParameter_OTAPLockout(bool mode, dn_ipmt_setParameter_OTAPLockout_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_OTAPLOCKOUT;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_OTAPLOCKOUT;
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_OTAPLOCKOUT_REQ_OFFS_MODE] = mode;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_OTAPLOCKOUT_REQ_LEN,                      // length
      dn_ipmt_setParameter_OTAPLockout_reply                    // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_OTAPLockout_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_OTAPLockout_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_OTAPLOCKOUT_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_OTAPLockout_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_routingMode

/**
This command allows the microprocessor to control whether the mote will become 
a router once joined the network. If disabled, the manager will keep the mote a 
leaf node. 
*/
dn_err_t dn_ipmt_setParameter_routingMode(bool mode, dn_ipmt_setParameter_routingMode_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_ROUTINGMODE;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_ROUTINGMODE;
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_ROUTINGMODE_REQ_OFFS_MODE] = mode;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_ROUTINGMODE_REQ_LEN,                      // length
      dn_ipmt_setParameter_routingMode_reply                    // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_routingMode_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_routingMode_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_ROUTINGMODE_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_routingMode_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_powerSrcInfo

/**
This command allows the microprocessor to configure power source information on 
the device. This setting is persistent and is used at network join time. 
*/
dn_err_t dn_ipmt_setParameter_powerSrcInfo(uint16_t maxStCurrent, uint8_t minLifetime, uint16_t currentLimit_0, uint16_t dischargePeriod_0, uint16_t rechargePeriod_0, uint16_t currentLimit_1, uint16_t dischargePeriod_1, uint16_t rechargePeriod_1, uint16_t currentLimit_2, uint16_t dischargePeriod_2, uint16_t rechargePeriod_2, dn_ipmt_setParameter_powerSrcInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_POWERSRCINFO;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_POWERSRCINFO;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_MAXSTCURRENT],maxStCurrent);
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_MINLIFETIME] = minLifetime;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_0],currentLimit_0);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_0],dischargePeriod_0);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_0],rechargePeriod_0);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_1],currentLimit_1);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_1],dischargePeriod_1);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_1],rechargePeriod_1);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_CURRENTLIMIT_2],currentLimit_2);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_DISCHARGEPERIOD_2],dischargePeriod_2);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_POWERSRCINFO_REQ_OFFS_RECHARGEPERIOD_2],rechargePeriod_2);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_POWERSRCINFO_REQ_LEN,                     // length
      dn_ipmt_setParameter_powerSrcInfo_reply                   // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_powerSrcInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_powerSrcInfo_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_POWERSRCINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_powerSrcInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_advKey

/**
Sets the Advertisement MIC key - this key is used to authenticate 
advertisements, and can be set per vendor/installation to prevent unauthorized 
devices from being able to respond to advertisements. If changed, it must match 
that set on the corresponding AP (using mset on the manager CLI) in order for 
the mote to join. It can be reset to default via the clearNV command. 
*/
dn_err_t dn_ipmt_setParameter_advKey(uint8_t* advKey, dn_ipmt_setParameter_advKey_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_ADVKEY;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_ADVKEY;
   memcpy(&dn_ipmt_vars.outputBuf[DN_SETPARAMETER_ADVKEY_REQ_OFFS_ADVKEY],advKey,16);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_ADVKEY_REQ_LEN,                           // length
      dn_ipmt_setParameter_advKey_reply                         // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_advKey_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_advKey_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_ADVKEY_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_advKey_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== setParameter_autoJoin

/**
This command allows the microprocessor to change between automatic and manual 
joining by the mote's networking stack. In manual mode, an explicit join 
command from the application is required to initiate joining. This setting is 
persistent and takes effect after mote reset.

Note that auto join mode must not be set if the application is also configured 
to join (e.g combining 'auto join' with 'master' mode will result in mote not 
joining). 
*/
dn_err_t dn_ipmt_setParameter_autoJoin(bool mode, dn_ipmt_setParameter_autoJoin_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_AUTOJOIN;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_AUTOJOIN;
   dn_ipmt_vars.outputBuf[DN_SETPARAMETER_AUTOJOIN_REQ_OFFS_MODE] = mode;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SETPARAMETER_AUTOJOIN_REQ_LEN,                         // length
      dn_ipmt_setParameter_autoJoin_reply                       // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_setParameter_autoJoin_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_setParameter_autoJoin_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SETPARAMETER_AUTOJOIN_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_setParameter_autoJoin_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_macAddress

/**
This command returns the MAC address of the device. By default, the MAC address 
returned is the EUI64 address of the device assigned by mote manufacturer, but 
it may be overwritten using the setParameter<macAddress> command. 
*/
dn_err_t dn_ipmt_getParameter_macAddress(dn_ipmt_getParameter_macAddress_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_MACADDRESS;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_MACADDRESS;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_MACADDRESS_REQ_LEN,                       // length
      dn_ipmt_getParameter_macAddress_reply                     // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_macAddress_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_macAddress_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_MACADDRESS_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_macAddress_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      memcpy(&reply->macAddress[0],&payload[DN_GETPARAMETER_MACADDRESS_REPLY_OFFS_MACADDRESS],8);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_networkId

/**
This command returns the network id stored in mote's persistent storage. 
*/
dn_err_t dn_ipmt_getParameter_networkId(dn_ipmt_getParameter_networkId_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_NETWORKID;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_NETWORKID;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_NETWORKID_REQ_LEN,                        // length
      dn_ipmt_getParameter_networkId_reply                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_networkId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_networkId_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_NETWORKID_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_networkId_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->networkId,&payload[DN_GETPARAMETER_NETWORKID_REPLY_OFFS_NETWORKID]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_txPower

/**
Get the radio output power in dBm, excluding any antenna gain. 
*/
dn_err_t dn_ipmt_getParameter_txPower(dn_ipmt_getParameter_txPower_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_TXPOWER;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_TXPOWER;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_TXPOWER_REQ_LEN,                          // length
      dn_ipmt_getParameter_txPower_reply                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_txPower_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_txPower_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_TXPOWER_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_txPower_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->txPower = (int8_t)payload[DN_GETPARAMETER_TXPOWER_REPLY_OFFS_TXPOWER];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_joinDutyCycle

/**
This command allows user to retrieve current value of joinDutyCycle parameter. 
*/
dn_err_t dn_ipmt_getParameter_joinDutyCycle(dn_ipmt_getParameter_joinDutyCycle_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_JOINDUTYCYCLE;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_JOINDUTYCYCLE;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_JOINDUTYCYCLE_REQ_LEN,                    // length
      dn_ipmt_getParameter_joinDutyCycle_reply                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_joinDutyCycle_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_joinDutyCycle_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_JOINDUTYCYCLE_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_joinDutyCycle_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->joinDutyCycle = payload[DN_GETPARAMETER_JOINDUTYCYCLE_REPLY_OFFS_JOINDUTYCYCLE];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_eventMask

/**
getParameter<eventMask> allows the microprocessor to read the currently 
subscribed-to event types. 
*/
dn_err_t dn_ipmt_getParameter_eventMask(dn_ipmt_getParameter_eventMask_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_EVENTMASK;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_EVENTMASK;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_EVENTMASK_REQ_LEN,                        // length
      dn_ipmt_getParameter_eventMask_reply                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_eventMask_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_eventMask_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_EVENTMASK_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_eventMask_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint32_t(&reply->eventMask,&payload[DN_GETPARAMETER_EVENTMASK_REPLY_OFFS_EVENTMASK]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_moteInfo

/**
The getParameter<moteInfo> command returns static information about the 
moteshardware and network stack software. 
*/
dn_err_t dn_ipmt_getParameter_moteInfo(dn_ipmt_getParameter_moteInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_MOTEINFO;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_MOTEINFO;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_MOTEINFO_REQ_LEN,                         // length
      dn_ipmt_getParameter_moteInfo_reply                       // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_moteInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_moteInfo_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_MOTEINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_moteInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->apiVersion = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_APIVERSION];
      memcpy(&reply->serialNumber[0],&payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SERIALNUMBER],8);
      reply->hwModel = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_HWMODEL];
      reply->hwRev = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_HWREV];
      reply->swVerMajor = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERMAJOR];
      reply->swVerMinor = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERMINOR];
      reply->swVerPatch = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERPATCH];
      dn_read_uint16_t(&reply->swVerBuild,&payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_SWVERBUILD]);
      reply->bootSwVer = payload[DN_GETPARAMETER_MOTEINFO_REPLY_OFFS_BOOTSWVER];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_netInfo

/**
The getParameter<networkInfo> command may be used to retrieve the mote's 
network-related parameters. 
*/
dn_err_t dn_ipmt_getParameter_netInfo(dn_ipmt_getParameter_netInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_NETINFO;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_NETINFO;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_NETINFO_REQ_LEN,                          // length
      dn_ipmt_getParameter_netInfo_reply                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_netInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_netInfo_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_NETINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_netInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      memcpy(&reply->macAddress[0],&payload[DN_GETPARAMETER_NETINFO_REPLY_OFFS_MACADDRESS],8);
      dn_read_uint16_t(&reply->moteId,&payload[DN_GETPARAMETER_NETINFO_REPLY_OFFS_MOTEID]);
      dn_read_uint16_t(&reply->networkId,&payload[DN_GETPARAMETER_NETINFO_REPLY_OFFS_NETWORKID]);
      dn_read_uint16_t(&reply->slotSize,&payload[DN_GETPARAMETER_NETINFO_REPLY_OFFS_SLOTSIZE]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_moteStatus

/**
The getParameter<moteStatus> command is used to retrieve current mote state 
andother dynamic information. 
*/
dn_err_t dn_ipmt_getParameter_moteStatus(dn_ipmt_getParameter_moteStatus_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_MOTESTATUS;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_MOTESTATUS;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_MOTESTATUS_REQ_LEN,                       // length
      dn_ipmt_getParameter_moteStatus_reply                     // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_moteStatus_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_moteStatus_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_MOTESTATUS_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_moteStatus_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->state = payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_STATE];
      reply->reserved_0 = payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_0];
      dn_read_uint16_t(&reply->reserved_1,&payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_1]);
      reply->numParents = payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_NUMPARENTS];
      dn_read_uint32_t(&reply->alarms,&payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_ALARMS]);
      reply->reserved_2 = payload[DN_GETPARAMETER_MOTESTATUS_REPLY_OFFS_RESERVED_2];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_time

/**
The getParameter<time> command may be used to request the current time on the 
mote. The mote reports time at the moment it is processing the command, so the 
information includes variable delay. For more precise time information consider 
using TIMEn pin (see timeIndication). 
*/
dn_err_t dn_ipmt_getParameter_time(dn_ipmt_getParameter_time_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_TIME;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_TIME;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_TIME_REQ_LEN,                             // length
      dn_ipmt_getParameter_time_reply                           // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_time_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_time_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_TIME_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_time_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint32_t(&reply->upTime,&payload[DN_GETPARAMETER_TIME_REPLY_OFFS_UPTIME]);
      memcpy(&reply->utcSecs[0],&payload[DN_GETPARAMETER_TIME_REPLY_OFFS_UTCSECS],8);
      dn_read_uint32_t(&reply->utcUsecs,&payload[DN_GETPARAMETER_TIME_REPLY_OFFS_UTCUSECS]);
      memcpy(&reply->asn[0],&payload[DN_GETPARAMETER_TIME_REPLY_OFFS_ASN],5);
      dn_read_uint16_t(&reply->asnOffset,&payload[DN_GETPARAMETER_TIME_REPLY_OFFS_ASNOFFSET]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_charge

/**
The getParameter<charge> command retrieves the charge consumption of the 
motesince the last reset. 
*/
dn_err_t dn_ipmt_getParameter_charge(dn_ipmt_getParameter_charge_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_CHARGE;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_CHARGE;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_CHARGE_REQ_LEN,                           // length
      dn_ipmt_getParameter_charge_reply                         // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_charge_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_charge_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_CHARGE_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_charge_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint32_t(&reply->qTotal,&payload[DN_GETPARAMETER_CHARGE_REPLY_OFFS_QTOTAL]);
      dn_read_uint32_t(&reply->upTime,&payload[DN_GETPARAMETER_CHARGE_REPLY_OFFS_UPTIME]);
      reply->tempInt = (int8_t)payload[DN_GETPARAMETER_CHARGE_REPLY_OFFS_TEMPINT];
      reply->tempFrac = payload[DN_GETPARAMETER_CHARGE_REPLY_OFFS_TEMPFRAC];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_testRadioRxStats

/**
The getParameter<testRadioRxStats> command retrieves statistics for the latest 
radio test performed using the testRadioRx command. The statistics show the 
number of good and bad packets (CRC failures) received during the test 
*/
dn_err_t dn_ipmt_getParameter_testRadioRxStats(dn_ipmt_getParameter_testRadioRxStats_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_TESTRADIORXSTATS;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_TESTRADIORXSTATS;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_TESTRADIORXSTATS_REQ_LEN,                 // length
      dn_ipmt_getParameter_testRadioRxStats_reply               // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_testRadioRxStats_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_testRadioRxStats_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_testRadioRxStats_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->rxOk,&payload[DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_OFFS_RXOK]);
      dn_read_uint16_t(&reply->rxFailed,&payload[DN_GETPARAMETER_TESTRADIORXSTATS_REPLY_OFFS_RXFAILED]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_OTAPLockout

/**
This command reads the current state of OTAP lockout, i.e. whether over-the-air 
upgrades of software are permitted on this mote. 
*/
dn_err_t dn_ipmt_getParameter_OTAPLockout(dn_ipmt_getParameter_OTAPLockout_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_OTAPLOCKOUT;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_OTAPLOCKOUT;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_OTAPLOCKOUT_REQ_LEN,                      // length
      dn_ipmt_getParameter_OTAPLockout_reply                    // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_OTAPLockout_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_OTAPLockout_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_OTAPLOCKOUT_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_OTAPLockout_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->mode = payload[DN_GETPARAMETER_OTAPLOCKOUT_REPLY_OFFS_MODE];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_moteId

/**
This command retrieves the mote's Mote ID. If the mote is not in the network, 
value of 0 is returned. 
*/
dn_err_t dn_ipmt_getParameter_moteId(dn_ipmt_getParameter_moteId_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_MOTEID;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_MOTEID;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_MOTEID_REQ_LEN,                           // length
      dn_ipmt_getParameter_moteId_reply                         // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_moteId_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_moteId_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_MOTEID_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_moteId_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->moteId,&payload[DN_GETPARAMETER_MOTEID_REPLY_OFFS_MOTEID]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_ipv6Address

/**
This command allows the microprocessor to read IPV6 address assigned to the 
mote. Before the mote has an assigned address it will return all 0s. 
*/
dn_err_t dn_ipmt_getParameter_ipv6Address(dn_ipmt_getParameter_ipv6Address_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_IPV6ADDRESS;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_IPV6ADDRESS;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_IPV6ADDRESS_REQ_LEN,                      // length
      dn_ipmt_getParameter_ipv6Address_reply                    // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_ipv6Address_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_ipv6Address_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_IPV6ADDRESS_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_ipv6Address_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      memcpy(&reply->ipv6Address[0],&payload[DN_GETPARAMETER_IPV6ADDRESS_REPLY_OFFS_IPV6ADDRESS],16);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_routingMode

/**
This command allows the microprocessor to retrieve the current routing mode of 
the mote. 
*/
dn_err_t dn_ipmt_getParameter_routingMode(dn_ipmt_getParameter_routingMode_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_ROUTINGMODE;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_ROUTINGMODE;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_ROUTINGMODE_REQ_LEN,                      // length
      dn_ipmt_getParameter_routingMode_reply                    // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_routingMode_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_routingMode_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_ROUTINGMODE_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_routingMode_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->routingMode = payload[DN_GETPARAMETER_ROUTINGMODE_REPLY_OFFS_ROUTINGMODE];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_appInfo

/**
Get the application (as opposed to the network stack) version information. 
*/
dn_err_t dn_ipmt_getParameter_appInfo(dn_ipmt_getParameter_appInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_APPINFO;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_APPINFO;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_APPINFO_REQ_LEN,                          // length
      dn_ipmt_getParameter_appInfo_reply                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_appInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_appInfo_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_APPINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_appInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->vendorId,&payload[DN_GETPARAMETER_APPINFO_REPLY_OFFS_VENDORID]);
      reply->appId = payload[DN_GETPARAMETER_APPINFO_REPLY_OFFS_APPID];
      memcpy(&reply->appVer[0],&payload[DN_GETPARAMETER_APPINFO_REPLY_OFFS_APPVER],5);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_powerSrcInfo

/**
This command allows the microprocessor to read a mote's power source settings. 
*/
dn_err_t dn_ipmt_getParameter_powerSrcInfo(dn_ipmt_getParameter_powerSrcInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_POWERSRCINFO;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_POWERSRCINFO;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_POWERSRCINFO_REQ_LEN,                     // length
      dn_ipmt_getParameter_powerSrcInfo_reply                   // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_powerSrcInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_powerSrcInfo_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_POWERSRCINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_powerSrcInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->maxStCurrent,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_MAXSTCURRENT]);
      reply->minLifetime = payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_MINLIFETIME];
      dn_read_uint16_t(&reply->currentLimit_0,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_0]);
      dn_read_uint16_t(&reply->dischargePeriod_0,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_0]);
      dn_read_uint16_t(&reply->rechargePeriod_0,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_0]);
      dn_read_uint16_t(&reply->currentLimit_1,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_1]);
      dn_read_uint16_t(&reply->dischargePeriod_1,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_1]);
      dn_read_uint16_t(&reply->rechargePeriod_1,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_1]);
      dn_read_uint16_t(&reply->currentLimit_2,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_CURRENTLIMIT_2]);
      dn_read_uint16_t(&reply->dischargePeriod_2,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_DISCHARGEPERIOD_2]);
      dn_read_uint16_t(&reply->rechargePeriod_2,&payload[DN_GETPARAMETER_POWERSRCINFO_REPLY_OFFS_RECHARGEPERIOD_2]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getParameter_autoJoin

/**
This command allows the microprocessor to retrieve the current autoJoin 
setting. 
*/
dn_err_t dn_ipmt_getParameter_autoJoin(dn_ipmt_getParameter_autoJoin_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETPARAMETER;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   dn_ipmt_vars.paramId        = PARAMID_AUTOJOIN;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[0] = PARAMID_AUTOJOIN;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETPARAMETER,                                       // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETPARAMETER_AUTOJOIN_REQ_LEN,                         // length
      dn_ipmt_getParameter_autoJoin_reply                       // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getParameter_autoJoin_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getParameter_autoJoin_rpt* reply;
   uint8_t paramId;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify I'm expecting this paramId
   paramId = payload[0];
   if (paramId!=dn_ipmt_vars.paramId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETPARAMETER_AUTOJOIN_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getParameter_autoJoin_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->autoJoin = payload[DN_GETPARAMETER_AUTOJOIN_REPLY_OFFS_AUTOJOIN];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== join

/**
The join command requests that mote start searching for the network and attempt 
to join.The mote must be in the Idle state or the Promiscuous Listen state(see 
search) for this command to be valid. Note that the join time will be affected 
by the maximum current setting. 
*/
dn_err_t dn_ipmt_join(dn_ipmt_join_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_JOIN;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_JOIN,                                               // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_JOIN_REQ_LEN,                                          // length
      dn_ipmt_join_reply                                        // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_join_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_join_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_join_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== disconnect

/**
The disconnect command requests that the mote initiate disconnection from the 
network. After disconnection completes, the mote will generate a disconnected 
event, and proceed to reset. If the mote is not in the network, the 
disconnected event will be generated immediately. This command may be issued at 
any time. 
*/
dn_err_t dn_ipmt_disconnect(dn_ipmt_disconnect_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_DISCONNECT;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_DISCONNECT,                                         // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_DISCONNECT_REQ_LEN,                                    // length
      dn_ipmt_disconnect_reply                                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_disconnect_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_disconnect_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_disconnect_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== reset

/**
The reset command initiates a soft-reset of the device. The device will 
initiate the reset sequence shortly after sending out the response to this 
command. Resetting a mote directly can adversely impact its descendants; to 
disconnect gracefully from the network, use the disconnect command 
*/
dn_err_t dn_ipmt_reset(dn_ipmt_reset_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_RESET;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_RESET,                                              // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_RESET_REQ_LEN,                                         // length
      dn_ipmt_reset_reply                                       // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_reset_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_reset_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_reset_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== lowPowerSleep

/**
The lowPowerSleep command shuts down all peripherals and places the mote into 
deep sleep mode. The command executes after the mote sends its response. The 
mote enters deep sleep within two seconds after the command executes. The 
command may be issued at any time and will cause the mote to interrupt all 
in-progress network operation. To achieve a graceful disconnect, use the 
disconnect command before using the lowPowerSleep command. A hardware reset is 
required to bring a mote out of deep sleep mode. 
*/
dn_err_t dn_ipmt_lowPowerSleep(dn_ipmt_lowPowerSleep_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_LOWPOWERSLEEP;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_LOWPOWERSLEEP,                                      // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_LOWPOWERSLEEP_REQ_LEN,                                 // length
      dn_ipmt_lowPowerSleep_reply                               // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_lowPowerSleep_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_lowPowerSleep_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_lowPowerSleep_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== testRadioRx

/**
The testRadioRx command clears all previously collected statistics and 
initiates a test of radio reception for the specified channel and duration. 
During the test, the mote keeps statistics about the number of packets received 
(with and without error). The test results may be retrieved using the 
getParameter<testRadioRxStats> command. The testRadioRx command may only be 
issued in Idle mode. The mote must be reset (either hardware or software reset) 
after radio tests are complete and prior to joining.

Station ID is available in IP mote >= 1.4, and WirelessHART mote >= 1.1.2. The 
station ID is a user selectable value used to isolate traffic if multiple tests 
are running in the same radio space. It must be set to match the station ID 
used by the transmitter.



Channel numbering is 0-15, corresponding to IEEE 2.4 GHz channels 11-26. 
*/
dn_err_t dn_ipmt_testRadioRx(uint16_t channelMask, uint16_t time, uint8_t stationId, dn_ipmt_testRadioRx_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_TESTRADIORX;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIORX_REQ_OFFS_CHANNELMASK],channelMask);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIORX_REQ_OFFS_TIME],time);
   dn_ipmt_vars.outputBuf[DN_TESTRADIORX_REQ_OFFS_STATIONID] = stationId;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_TESTRADIORX,                                        // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_TESTRADIORX_REQ_LEN,                                   // length
      dn_ipmt_testRadioRx_reply                                 // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_testRadioRx_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_testRadioRx_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_testRadioRx_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== clearNV

/**
The clearNV command resets the motes non-volatile memory (NV) to its 
factory-default state. See User Guide for detailed information about the 
default values. Since many parameters are read by the mote only at power-up, 
this command should be followed up by mote reset. 
*/
dn_err_t dn_ipmt_clearNV(dn_ipmt_clearNV_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_CLEARNV;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_CLEARNV,                                            // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_CLEARNV_REQ_LEN,                                       // length
      dn_ipmt_clearNV_reply                                     // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_clearNV_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_clearNV_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_clearNV_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== requestService

/**
The requestService command may be used to request a new or changed service 
level to a destination device in the mesh. This command may only be used to 
update the service to a device with an existing connection (session).

Whenever a change in bandwidth assignment occurs, the application receives a 
serviceChanged event that it can use as a trigger to read the new service 
allocation. 
*/
dn_err_t dn_ipmt_requestService(uint16_t destAddr, uint8_t serviceType, uint32_t value, dn_ipmt_requestService_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_REQUESTSERVICE;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_REQUESTSERVICE_REQ_OFFS_DESTADDR],destAddr);
   dn_ipmt_vars.outputBuf[DN_REQUESTSERVICE_REQ_OFFS_SERVICETYPE] = serviceType;
   dn_write_uint32_t(&dn_ipmt_vars.outputBuf[DN_REQUESTSERVICE_REQ_OFFS_VALUE],value);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_REQUESTSERVICE,                                     // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_REQUESTSERVICE_REQ_LEN,                                // length
      dn_ipmt_requestService_reply                              // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_requestService_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_requestService_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_requestService_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== getServiceInfo

/**
The getServiceInfo command returns information about the service currently 
allocated to the mote. 
*/
dn_err_t dn_ipmt_getServiceInfo(uint16_t destAddr, uint8_t type, dn_ipmt_getServiceInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_GETSERVICEINFO;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_GETSERVICEINFO_REQ_OFFS_DESTADDR],destAddr);
   dn_ipmt_vars.outputBuf[DN_GETSERVICEINFO_REQ_OFFS_TYPE] = type;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_GETSERVICEINFO,                                     // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_GETSERVICEINFO_REQ_LEN,                                // length
      dn_ipmt_getServiceInfo_reply                              // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_getServiceInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_getServiceInfo_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_GETSERVICEINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_getServiceInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      dn_read_uint16_t(&reply->destAddr,&payload[DN_GETSERVICEINFO_REPLY_OFFS_DESTADDR]);
      reply->type = payload[DN_GETSERVICEINFO_REPLY_OFFS_TYPE];
      reply->state = payload[DN_GETSERVICEINFO_REPLY_OFFS_STATE];
      dn_read_uint32_t(&reply->value,&payload[DN_GETSERVICEINFO_REPLY_OFFS_VALUE]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== openSocket

/**
The openSocket command creates an endpoint for IP communication and returns an 
ID for the socket. 
*/
dn_err_t dn_ipmt_openSocket(uint8_t protocol, dn_ipmt_openSocket_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_OPENSOCKET;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_OPENSOCKET_REQ_OFFS_PROTOCOL] = protocol;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_OPENSOCKET,                                         // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_OPENSOCKET_REQ_LEN,                                    // length
      dn_ipmt_openSocket_reply                                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_openSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_openSocket_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_OPENSOCKET_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_openSocket_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->socketId = payload[DN_OPENSOCKET_REPLY_OFFS_SOCKETID];
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== closeSocket

/**
Close the previously open socket. 
*/
dn_err_t dn_ipmt_closeSocket(uint8_t socketId, dn_ipmt_closeSocket_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_CLOSESOCKET;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_CLOSESOCKET_REQ_OFFS_SOCKETID] = socketId;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_CLOSESOCKET,                                        // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_CLOSESOCKET_REQ_LEN,                                   // length
      dn_ipmt_closeSocket_reply                                 // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_closeSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_closeSocket_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_closeSocket_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== bindSocket

/**
Bind a previously opened socket to a port. When a socket is created, it is only 
given a protocol family, but not assigned a port. This association must be 
performed before the socket can accept connections from other hosts. 
*/
dn_err_t dn_ipmt_bindSocket(uint8_t socketId, uint16_t port, dn_ipmt_bindSocket_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_BINDSOCKET;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_BINDSOCKET_REQ_OFFS_SOCKETID] = socketId;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_BINDSOCKET_REQ_OFFS_PORT],port);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_BINDSOCKET,                                         // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_BINDSOCKET_REQ_LEN,                                    // length
      dn_ipmt_bindSocket_reply                                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_bindSocket_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_bindSocket_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_bindSocket_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== sendTo

/**
Send a packet into the network. If the command returns RC_OK, the mote has 
accepted the packet and hasqueuedit up for transmission. A txDone notification 
will be issued when the packet has been sent, if and only if the packet ID 
passed in this command is different from 0xffff. You can set the packet ID to 
any value. The notification will contain the packet ID of the packet just sent, 
allowing association of the notification with a particular packet. The 
destination port should be in the range 0xF0B8-F0BF (61624-61631) to maximize 
payload. 
*/
dn_err_t dn_ipmt_sendTo(uint8_t socketId, uint8_t* destIP, uint16_t destPort, uint8_t serviceType, uint8_t priority, uint16_t packetId, uint8_t* payload, uint8_t payloadLen, dn_ipmt_sendTo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SENDTO;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_SOCKETID] = socketId;
   memcpy(&dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_DESTIP],destIP,16);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_DESTPORT],destPort);
   dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_SERVICETYPE] = serviceType;
   dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_PRIORITY] = priority;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_PACKETID],packetId);
   memcpy(&dn_ipmt_vars.outputBuf[DN_SENDTO_REQ_OFFS_PAYLOAD],payload,payloadLen);
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SENDTO,                                             // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SENDTO_REQ_LEN+payloadLen,                             // length
      dn_ipmt_sendTo_reply                                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_sendTo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_sendTo_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_sendTo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== search

/**
The search command requests that mote start listening for advertisements and 
report those heard from any network withoutattempting to join. This is called 
the Promiscuous Listen state. The mote must be in the Idle state for this 
command to be valid. The search state can be exited by issuing the join command 
or the reset command. 
*/
dn_err_t dn_ipmt_search(dn_ipmt_search_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SEARCH;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SEARCH,                                             // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SEARCH_REQ_LEN,                                        // length
      dn_ipmt_search_reply                                      // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_search_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_search_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_search_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== testRadioTxExt

/**
The testRadioTxExt command allows the microprocessor to initiate a radio 
transmission test. This command may only be issued prior to the mote joining 
the network. Three types of transmission tests are supported:

- Packet transmission
- Continuous modulation
- Continuous wave (unmodulated signal)

In a packet transmission test, the mote generates a repeatCnt number of packet 
sequences. Each sequence consists of up to 10 packets with configurable size 
and delays. Each packet starts with a PHY preamble (5 bytes), followed by a PHY 
length field (1 byte), followed by data payload of up to 125 bytes, and finally 
a 2-byte 802.15.4 CRC at the end. Byte 0 of the payload contains stationId of 
the sender. Bytes 1 and 2 contain the packet number (in big-endian format) that 
increments with every packet transmitted. Bytes 3..N contain a counter (from 
0..N-3) that increments with every byte inside payload. Transmissions occur on 
the set of channels defined by chanMask , selected inpseudo-randomorder.

In a continuous modulation test, the mote generates continuous pseudo-random 
modulated signal, centered at the specified channel. The test is stopped by 
resetting the mote.

In a continuous wave test, the mote generates an unmodulated tone, centered at 
the specified channel. The test tone is stopped by resetting the mote.

The testRadioTxExt command may only be issued when the mote is in Idle mode, 
prior to its joining the network. The mote must be reset (either hardware or 
software reset) after radio tests are complete and prior to joining.

The station ID is a user selectable value. It is used in packet tests so that a 
receiver can identify packets from this device in cases where there may be 
multiple tests running in the same radio space. This field is not used for CM 
or CW tests. See testRadioRX (SmartMesh IP) or testRadioRxExt (SmartMesh WirelessHART).



Channel numbering is 0-15, corresponding to IEEE 2.4 GHz channels 11-26. 
*/
dn_err_t dn_ipmt_testRadioTxExt(uint8_t testType, uint16_t chanMask, uint16_t repeatCnt, int8_t txPower, uint8_t seqSize, uint8_t pkLen_1, uint16_t delay_1, uint8_t pkLen_2, uint16_t delay_2, uint8_t pkLen_3, uint16_t delay_3, uint8_t pkLen_4, uint16_t delay_4, uint8_t pkLen_5, uint16_t delay_5, uint8_t pkLen_6, uint16_t delay_6, uint8_t pkLen_7, uint16_t delay_7, uint8_t pkLen_8, uint16_t delay_8, uint8_t pkLen_9, uint16_t delay_9, uint8_t pkLen_10, uint16_t delay_10, uint8_t stationId, dn_ipmt_testRadioTxExt_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_TESTRADIOTXEXT;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_TESTTYPE] = testType;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_CHANMASK],chanMask);
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_REPEATCNT],repeatCnt);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_TXPOWER] = (int8_t)txPower;
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_SEQSIZE] = seqSize;
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_1] = pkLen_1;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_1],delay_1);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_2] = pkLen_2;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_2],delay_2);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_3] = pkLen_3;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_3],delay_3);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_4] = pkLen_4;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_4],delay_4);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_5] = pkLen_5;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_5],delay_5);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_6] = pkLen_6;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_6],delay_6);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_7] = pkLen_7;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_7],delay_7);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_8] = pkLen_8;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_8],delay_8);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_9] = pkLen_9;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_9],delay_9);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_PKLEN_10] = pkLen_10;
   dn_write_uint16_t(&dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_DELAY_10],delay_10);
   dn_ipmt_vars.outputBuf[DN_TESTRADIOTXEXT_REQ_OFFS_STATIONID] = stationId;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_TESTRADIOTXEXT,                                     // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_TESTRADIOTXEXT_REQ_LEN,                                // length
      dn_ipmt_testRadioTxExt_reply                              // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_testRadioTxExt_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_testRadioTxExt_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_testRadioTxExt_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== zeroize

/**
Zeroize (zeroise) command erases flash area that is used to store configuration 
parameters, such as join keys. This command is intended to satisfy zeroization 
requirement of FIPS-140 standard. After the command executes, the mote should 
be reset. Available in mote >= 1.4.x

The zeroize command will render the mote inoperable. It must be re-programmed 
via SPI or JTAG in order to be useable. 
*/
dn_err_t dn_ipmt_zeroize(dn_ipmt_zeroize_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_ZEROIZE;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_ZEROIZE,                                            // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_ZEROIZE_REQ_LEN,                                       // length
      dn_ipmt_zeroize_reply                                     // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_zeroize_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_zeroize_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // do NOT verify length (no return fields expected)
   
   // cast the replyContent
   reply = (dn_ipmt_zeroize_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//===== socketInfo

/**
Retrieve information about a socket. (Available in IP Mote >= 1.4.0) 
*/
dn_err_t dn_ipmt_socketInfo(uint8_t index, dn_ipmt_socketInfo_rpt* reply) {
   uint8_t    extraFlags;
   dn_err_t   rc;
   
   // lock the module
   dn_lock();
   
   // verify no ongoing transmissions
   if (dn_ipmt_vars.busyTx) {
      // unlock the module
      dn_unlock();
      
      // return
      return DN_ERR_BUSY;
   }
   
   // store callback information
   dn_ipmt_vars.cmdId          = CMDID_SOCKETINFO;
   dn_ipmt_vars.replyContents  = (uint8_t*)reply;
   
   // extraFlags
   extraFlags = 0x00;
   
   // build outputBuf
   dn_ipmt_vars.outputBuf[DN_SOCKETINFO_REQ_OFFS_INDEX] = index;
   
   // send outputBuf
   rc = dn_serial_mt_sendRequest(
      CMDID_SOCKETINFO,                                         // cmdId
      extraFlags,                                               // extraFlags
      dn_ipmt_vars.outputBuf,                                   // payload
      DN_SOCKETINFO_REQ_LEN,                                    // length
      dn_ipmt_socketInfo_reply                                  // replyCb
   );
   
   if (rc==DN_ERR_NONE) {
      // I'm now busy transmitting
      dn_ipmt_vars.busyTx         = TRUE;
   }
   
   // unlock the module
   dn_unlock();
   
   return rc;
   
}

void dn_ipmt_socketInfo_reply(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len) {
   dn_ipmt_socketInfo_rpt* reply;
   
   // verify I'm expecting this answer
   if (dn_ipmt_vars.busyTx==FALSE || dn_ipmt_vars.cmdId!=cmdId) {
      return;
   }
   
   // verify length
   if (rc==DN_SERIAL_RC_OK && len<DN_SOCKETINFO_REPLY_LEN) {
      return;
   }
   
   // cast the replyContent
   reply = (dn_ipmt_socketInfo_rpt*)dn_ipmt_vars.replyContents;
   
   // store RC
   reply->RC = rc;
   
   // parse returned value (iff RC==0)
   if (rc==DN_SERIAL_RC_OK) {
      
      reply->index = payload[DN_SOCKETINFO_REPLY_OFFS_INDEX];
      reply->socketId = payload[DN_SOCKETINFO_REPLY_OFFS_SOCKETID];
      reply->protocol = payload[DN_SOCKETINFO_REPLY_OFFS_PROTOCOL];
      reply->bindState = payload[DN_SOCKETINFO_REPLY_OFFS_BINDSTATE];
      dn_read_uint16_t(&reply->port,&payload[DN_SOCKETINFO_REPLY_OFFS_PORT]);
   }
   
   // call the callback
   dn_ipmt_vars.replyCb(cmdId);
   
   // I'm not busy transmitting anymore
   dn_ipmt_vars.busyTx=FALSE;
}

//========== serialRX

void dn_ipmt_rxSerialRequest(uint8_t cmdId, uint8_t flags, uint8_t* payload, uint8_t len) {
   dn_ipmt_timeIndication_nt* notif_timeIndication;
   dn_ipmt_events_nt* notif_events;
   dn_ipmt_receive_nt* notif_receive;
   dn_ipmt_macRx_nt* notif_macRx;
   dn_ipmt_txDone_nt* notif_txDone;
   dn_ipmt_advReceived_nt* notif_advReceived;
   
   // parse notification
   switch(cmdId) {
      case CMDID_TIMEINDICATION:
         
         // verify length payload received
         if (len<23) {
            return;
         }
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_timeIndication = (dn_ipmt_timeIndication_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         dn_read_uint32_t(&notif_timeIndication->uptime,&payload[DN_TIMEINDICATION_NOTIF_OFFS_UPTIME]);
         memcpy(&notif_timeIndication->utcSecs[0],&payload[DN_TIMEINDICATION_NOTIF_OFFS_UTCSECS],8);
         dn_read_uint32_t(&notif_timeIndication->utcUsecs,&payload[DN_TIMEINDICATION_NOTIF_OFFS_UTCUSECS]);
         memcpy(&notif_timeIndication->asn[0],&payload[DN_TIMEINDICATION_NOTIF_OFFS_ASN],5);
         dn_read_uint16_t(&notif_timeIndication->asnOffset,&payload[DN_TIMEINDICATION_NOTIF_OFFS_ASNOFFSET]);
         break;
      case CMDID_EVENTS:
         
         // verify length payload received
         if (len<9) {
            return;
         }
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_events = (dn_ipmt_events_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         dn_read_uint32_t(&notif_events->events,&payload[DN_EVENTS_NOTIF_OFFS_EVENTS]);
         notif_events->state = payload[DN_EVENTS_NOTIF_OFFS_STATE];
         dn_read_uint32_t(&notif_events->alarmsList,&payload[DN_EVENTS_NOTIF_OFFS_ALARMSLIST]);
         break;
      case CMDID_RECEIVE:
         
         // verify length payload received
         if (len<19) {
            return;
         }
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_receive = (dn_ipmt_receive_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         notif_receive->socketId = payload[DN_RECEIVE_NOTIF_OFFS_SOCKETID];
         memcpy(&notif_receive->srcAddr[0],&payload[DN_RECEIVE_NOTIF_OFFS_SRCADDR],16);
         dn_read_uint16_t(&notif_receive->srcPort,&payload[DN_RECEIVE_NOTIF_OFFS_SRCPORT]);
         notif_receive->payloadLen = len-DN_RECEIVE_NOTIF_OFFS_PAYLOAD;
         memcpy(&notif_receive->payload[0],&payload[DN_RECEIVE_NOTIF_OFFS_PAYLOAD],len-DN_RECEIVE_NOTIF_OFFS_PAYLOAD);
         break;
      case CMDID_MACRX:
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_macRx = (dn_ipmt_macRx_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         memcpy(&notif_macRx->payload[0],&payload[DN_MACRX_NOTIF_OFFS_PAYLOAD],len-DN_MACRX_NOTIF_OFFS_PAYLOAD);
         break;
      case CMDID_TXDONE:
         
         // verify length payload received
         if (len<3) {
            return;
         }
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_txDone = (dn_ipmt_txDone_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         dn_read_uint16_t(&notif_txDone->packetId,&payload[DN_TXDONE_NOTIF_OFFS_PACKETID]);
         notif_txDone->status = payload[DN_TXDONE_NOTIF_OFFS_STATUS];
         break;
      case CMDID_ADVRECEIVED:
         
         // verify length payload received
         if (len<6) {
            return;
         }
         
         // verify length notifBuf
         if (len>dn_ipmt_vars.notifBufLen) {
            return;
         }
         
         // cast notifBuf
         notif_advReceived = (dn_ipmt_advReceived_nt*)dn_ipmt_vars.notifBuf;
         
         // parse the notification
         dn_read_uint16_t(&notif_advReceived->netId,&payload[DN_ADVRECEIVED_NOTIF_OFFS_NETID]);
         dn_read_uint16_t(&notif_advReceived->moteId,&payload[DN_ADVRECEIVED_NOTIF_OFFS_MOTEID]);
         notif_advReceived->rssi = (int8_t)payload[DN_ADVRECEIVED_NOTIF_OFFS_RSSI];
         notif_advReceived->joinPri = payload[DN_ADVRECEIVED_NOTIF_OFFS_JOINPRI];
         break;
      default:
         // unknown cmdID
         return;
   }
   
   // call the callback
   dn_ipmt_vars.notifCb(cmdId,DN_SUBCMDID_NONE);
}

//=========================== helpers =========================================

