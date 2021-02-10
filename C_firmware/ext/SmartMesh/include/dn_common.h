/*
Copyright (c) 2014, Dust Networks. All rights reserved.

Commmon definitions.

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_COMMON_H
#define DN_COMMON_H

#include "inttypes.h"
#include <stdbool.h>
#include <string.h>

//=========================== defines =========================================

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// error codes
typedef enum {
   DN_ERR_NONE = 0,
   DN_ERR_BUSY, 
   DN_ERR_NOT_CONNECTED, // only used in SmartMesh IP Manager
   DN_ERR_ALREADY,
   DN_ERR_MALFORMED
} dn_err_t;

//=== API return type

//=========================== typedef =========================================

#endif
