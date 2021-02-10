/*
Copyright (c) 2014, Dust Networks. All rights reserved.

\license See attached DN_LICENSE.txt.
*/

#ifndef DN_UART_H
#define DN_UART_H

#include "dn_common.h"
#include <adi_processor.h>

//=========================== defined =========================================
#define buffer_size      128
#define DATASIZE_i2c     8

/* Memory required by the driver for bidirectional mode of operation. */
#define ADI_UART_MEMORY_SIZE    (ADI_UART_BIDIR_MEMORY_SIZE)

/* Size of the data buffers that will be processed. */
#define SIZE_OF_BUFFER  26u

/* UART device number. There are 2 devices for ADuCM4050, so this can be 0 or 1.
   But for ADuCM302x this should be '0'*/
#if defined (__ADUCM302x__)
#define 	UART_DEVICE_NUM 0u
#elif defined(__ADUCM4x50__)
#ifdef         COG 
#define        UART_DEVICE_NUM 0u
#define        RESET_SM_PORT   ADI_GPIO_PORT2
#define        RESET_SM_PIN    ADI_GPIO_PIN_9 
#else
#define        UART_DEVICE_NUM 1u
#define        RESET_SM_PORT   ADI_GPIO_PORT0
#define        RESET_SM_PIN    ADI_GPIO_PIN_14 
#endif  /* End of #ifdef COG*/
#else
#error UART driver is not ported for this processor
#endif /*End of #if defined.... #elif*/

/* Timeout value for receiving data. */
#define UART_GET_BUFFER_TIMEOUT 1000000u
//=========================== typedef =========================================

typedef void (*dn_uart_rxByte_cbt)(uint8_t byte);

//=========================== variables =======================================


//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb);
void dn_uart_txByte(uint8_t byte);
void dn_uart_txFlush();


#ifdef __cplusplus
}
#endif

#endif
