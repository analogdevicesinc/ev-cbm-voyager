/*
Copyright (c) 2014, Dust Networks. All rights reserved.

HDLC library.

\license See attached DN_LICENSE.txt.
*/

#include "dn_serial_mt.h"
#include "dn_hdlc.h"

//=========================== variables =======================================

typedef struct {
   // packet IDs
   uint8_t                   txPacketId;
   uint8_t                   rxPacketId;
   // reply callback
   uint8_t                   replyCmdId;
   dn_serial_reply_cbt       replyCb;
   // request callback
   dn_serial_request_cbt     requestCb;
} dn_serial_mt_vars_t;

dn_serial_mt_vars_t dn_serial_mt_vars;

//=========================== prototype =======================================

void dn_serial_mt_rxHdlcFrame(uint8_t* rxFrame, uint8_t rxFrameLen);
void dn_serial_mt_dispatch_response(uint8_t cmdId, uint8_t *payload, uint8_t length);
void dn_serial_sendReply(uint8_t cmdId, uint8_t rc, uint8_t *payload, uint8_t length);

//=========================== public ==========================================

/**
\brief Setting up the instance.
*/
void dn_serial_mt_init(dn_serial_request_cbt requestCb) {
   // reset local variables
   memset(&dn_serial_mt_vars, 0, sizeof(dn_serial_mt_vars));
   
   // initialize variables
   dn_serial_mt_vars.txPacketId = DN_SERIAL_PACKETID_NOTSET;
   dn_serial_mt_vars.rxPacketId = DN_SERIAL_PACKETID_NOTSET;
   dn_serial_mt_vars.requestCb  = requestCb;
   
   // initialize the HDLC module
   dn_hdlc_init(dn_serial_mt_rxHdlcFrame);
}

dn_err_t dn_serial_mt_sendRequest(uint8_t cmdId,  uint8_t extraFlags, uint8_t* payload, uint8_t length, dn_serial_reply_cbt replyCb) {
   uint8_t i;
   uint8_t flags;
   
   // register reply callback
   dn_serial_mt_vars.replyCmdId      = cmdId;
   dn_serial_mt_vars.replyCb         = replyCb;
   
   // calculate the flags
   flags      = 0;
   if (dn_serial_mt_vars.txPacketId==DN_SERIAL_PACKETID_NOTSET) {
      dn_serial_mt_vars.txPacketId = 0x00;
      flags  |= 0x01<<3;                          // sync bit
      flags  |= dn_serial_mt_vars.txPacketId<<1;     // txPacketId
   } else {
      flags  |= dn_serial_mt_vars.txPacketId<<1;     // txPacketId
   }
   flags     |= extraFlags;
   
   // send the frame over serial
   dn_hdlc_outputOpen();
   dn_hdlc_outputWrite(cmdId);                   // cmdId
   dn_hdlc_outputWrite(length);                  // length
   dn_hdlc_outputWrite(flags);                   // flags
   for (i=0; i<length; i++) {                    // payload
      dn_hdlc_outputWrite(payload[i]);
   }
   dn_hdlc_outputClose();
   
   // toggle the txPacketId
   if (dn_serial_mt_vars.txPacketId==0x00) {
      dn_serial_mt_vars.txPacketId   = 0x01;
   } else {
      dn_serial_mt_vars.txPacketId   = 0x00;
   }
   
   return DN_ERR_NONE;
}

//=========================== private =========================================

void dn_serial_mt_rxHdlcFrame(uint8_t* rxFrame, uint8_t rxFrameLen) {
   // fields in the serial API header
   uint8_t cmdId;
   uint8_t length;
   uint8_t flags;
   uint8_t isResponse;
   uint8_t packetId;
   uint8_t isSync;
   // misc
   uint8_t isRepeatId;
   
   // assert length is OK
   if (rxFrameLen<3) {
      return;
   }
   
   // parse header
   cmdId      = rxFrame[0];
   length     = rxFrame[1];
   flags      = rxFrame[2];
   isResponse = ((flags & DN_SERIAL_API_MASK_RESPONSE)!=0);
   packetId   = ((flags & DN_SERIAL_API_MASK_PACKETID)!=0);
   isSync     = ((flags & DN_SERIAL_API_MASK_SYNC)!=0);
   
   // check if valid packet ID
   if (isResponse) {
      // dispatch
      
      dn_serial_mt_dispatch_response(cmdId,&rxFrame[3],length);
   } else {
      if (isSync || packetId!=dn_serial_mt_vars.rxPacketId) {
         isRepeatId          = FALSE;
         dn_serial_mt_vars.rxPacketId = packetId;
      } else {
         isRepeatId          = TRUE;
      }
      
      // ACK
      dn_serial_sendReply(cmdId,DN_SERIAL_RC_OK,NULL,0);
      
      // dispatch
      if (isRepeatId==FALSE && length>0 && dn_serial_mt_vars.requestCb!=NULL) {
         dn_serial_mt_vars.requestCb(cmdId,flags,&rxFrame[3],length);
      }
   }
}

/**
\note Not public, only used for sending ACK.
*/
void dn_serial_sendReply(uint8_t cmdId, uint8_t rc, uint8_t *payload, uint8_t length) {
   uint8_t i;
   
   dn_hdlc_outputOpen();
   dn_hdlc_outputWrite(cmdId);                                                    // cmdId
   dn_hdlc_outputWrite(length);                                                   // length
   dn_hdlc_outputWrite(DN_SERIAL_API_MASK_RESPONSE | (dn_serial_mt_vars.rxPacketId<<1));   // flags
   dn_hdlc_outputWrite(rc);                                                       // rc
   for (i=0; i<length; i++) {                                                  // payload
      dn_hdlc_outputWrite(payload[i]);
   }
   dn_hdlc_outputClose();
}

void dn_serial_mt_dispatch_response(uint8_t cmdId, uint8_t* payload, uint8_t length) {
   uint8_t rc;
   
   rc = payload[0];
   if (cmdId==dn_serial_mt_vars.replyCmdId && dn_serial_mt_vars.replyCb!=NULL) {
      
      // call the callback
      (dn_serial_mt_vars.replyCb)(cmdId,rc,&payload[1],length);
      
      // reset
      dn_serial_mt_vars.replyCmdId   = 0x00;
      dn_serial_mt_vars.replyCb      = NULL;
   }
}

//=========================== helpers =========================================
