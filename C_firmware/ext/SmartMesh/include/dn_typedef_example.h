#ifndef __DN_TYPEDEF_H
#define __DN_TYPEDEF_H
typedef unsigned char  INT8U;
typedef          char  INT8S;
typedef unsigned short INT16U;
typedef          short INT16S;
typedef unsigned int   INT32U;
typedef          int   INT32S;
typedef long     long  INT64S;
typedef unsigned char  BOOLEAN;

#define  PACKED_START  __pragma(pack(1))
#define  PACKED_STOP   __pragma(pack())
#if !defined(L_ENDIAN) && !defined(B_ENDIAN)
    #define L_ENDIAN
#endif 

#endif
