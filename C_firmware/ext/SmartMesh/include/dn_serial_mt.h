/*
Copyright (c) 2014, Dust Networks. All rights reserved.

Serial connector.

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_SERIAL_H
#define DN_SERIAL_H

#include "dn_common.h"

//=========================== defines =========================================

#define DN_SERIAL_API_MASK_RESPONSE    0x01
#define DN_SERIAL_API_MASK_PACKETID    0x02
#define DN_SERIAL_API_MASK_SYNC        0x08

#define DN_SERIAL_PACKETID_NOTSET      0x02

// return code
#define DN_SERIAL_RC_OK                0x00

//=========================== typedef =========================================

typedef void (*dn_serial_request_cbt)(uint8_t cmdId, uint8_t flags, uint8_t* payload, uint8_t len);
typedef void (*dn_serial_reply_cbt)(uint8_t cmdId, uint8_t rc, uint8_t* payload, uint8_t len);

//=========================== variables =======================================

//=========================== prototypes ======================================

void     dn_serial_mt_init(dn_serial_request_cbt requestCb);
dn_err_t dn_serial_mt_sendRequest(
   uint8_t              cmdId,
   uint8_t              extraFlags,
   uint8_t*             payload,
   uint8_t              length,
   dn_serial_reply_cbt  replyCb
);

#endif
