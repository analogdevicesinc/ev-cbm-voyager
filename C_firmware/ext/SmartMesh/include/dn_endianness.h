/*
Copyright (c) 2014, Dust Networks. All rights reserved.

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_ENDIANNESS_H
#define DN_ENDIANNESS_H

#include "dn_common.h"

//=========================== defined =========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

void dn_write_uint16_t(uint8_t* ptr, uint16_t val);
void dn_write_uint32_t(uint8_t* ptr, uint32_t val);
void dn_write_int32_t(uint8_t* ptr,  int32_t val);
void dn_read_uint16_t(uint16_t* to, uint8_t* from);
void dn_read_uint32_t(uint32_t* to, uint8_t* from);
void dn_read_int32_t(int32_t* to, uint8_t* from);

#ifdef __cplusplus
}
#endif

#endif
