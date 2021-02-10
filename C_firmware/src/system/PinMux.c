/*
 **
 ** Source file generated on February 20, 2018 at 14:51:55.	
 **
 ** Copyright (C) 2018 Analog Devices Inc., All Rights Reserved.
 **
 ** This file is generated automatically based upon the options selected in 
 ** the Pin Multiplexing configuration editor. Changes to the Pin Multiplexing
 ** configuration should be made by changing the appropriate options rather
 ** than editing this file.
 **
 ** Selected Peripherals
 ** --------------------
 ** UART0 (Tx, Rx, SOUT_EN)
 ** ADC0_IN (ADC0_VIN0, ADC0_VIN1, ADC0_VIN2)
 ** UART1 (Tx, Rx)
 ** SPI0 (SCLK, MOSI, MISO, CS_1)
 ** SPI1 (SCLK, MOSI, MISO)
 **
 ** GPIO (unavailable)
 ** ------------------
 ** P0_10, P0_11, P0_12, 
 ** P2_03, P2_04, P2_05, 
 ** P1_15, P2_00
 ** P0_00, P0_01, P0_02, P1_10
 ** P1_06, P1_07, P1_08
*/

#include <sys/platform.h>
#include <stdint.h>

#ifdef COG 
#define UART0_TX_PORTP0_MUX  ((uint32_t) ((uint32_t) 1<<20))
#define UART0_RX_PORTP0_MUX  ((uint32_t) ((uint32_t) 1<<22))
#define UART0_SOUT_EN_PORTP0_MUX  ((uint32_t) ((uint32_t) 3<<24))
#endif
   
#define UART1_TX_PORTP1_MUX  ((uint32_t) ((uint32_t) 2<<30))
#define UART1_RX_PORTP2_PIN0_MUX  ((uint16_t) ((uint16_t) 2<<0))

#define ADC0_IN_ADC0_VIN0_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<6))
//#define ADC0_IN_ADC0_VIN1_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<8))
//#define ADC0_IN_ADC0_VIN2_PORTP2_MUX  ((uint16_t) ((uint16_t) 1<<10))

/* 02/27/2019  Fan added for SPI0 */ 
#define SPI0_SCLK_PORTP0_MUX  ((uint16_t) ((uint16_t) 1<<0))
#define SPI0_MOSI_PORTP0_MUX  ((uint16_t) ((uint16_t) 1<<2))
#define SPI0_MISO_PORTP0_MUX  ((uint16_t) ((uint16_t) 1<<4))
#define SPI0_CS_1_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<20))
/* added end */

/* 03/27/2019  Fan added for SPI1 */ 
#define SPI1_SCLK_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<12))
#define SPI1_MOSI_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<14))
#define SPI1_MISO_PORTP1_MUX  ((uint32_t) ((uint32_t) 1<<16))
/* added end */


int32_t adi_initpinmux(void);

/*
 * Initialize the Port Control MUX Registers
 */
int32_t adi_initpinmux(void) {
    /* PORTx_MUX registers */
#ifdef COG
    *pREG_GPIO0_CFG = UART0_TX_PORTP0_MUX | UART0_RX_PORTP0_MUX
     | UART0_SOUT_EN_PORTP0_MUX;
#endif
    /* Port Control MUX registers */
    *pREG_GPIO1_CFG = UART1_TX_PORTP1_MUX;
//  *pREG_GPIO2_CFG = ADC0_IN_ADC0_VIN0_PORTP2_MUX | ADC0_IN_ADC0_VIN1_PORTP2_MUX
//                  | ADC0_IN_ADC0_VIN2_PORTP2_MUX | UART1_RX_PORTP2_PIN0_MUX;
    *pREG_GPIO2_CFG = ADC0_IN_ADC0_VIN0_PORTP2_MUX | UART1_RX_PORTP2_PIN0_MUX;

/* 02/27/2019  Fan added for PORT0_MUX to Function1, SPI0 */ 
    *((volatile uint32_t *)REG_GPIO0_CFG) |= SPI0_SCLK_PORTP0_MUX | SPI0_MOSI_PORTP0_MUX 
                                           | SPI0_MISO_PORTP0_MUX;
    *pREG_GPIO1_CFG |= SPI0_CS_1_PORTP1_MUX;
/* added end */

/* 03/27/2019  Fan added for PORT1_MUX to Function1, SPI1 */ 
    *pREG_GPIO1_CFG |= SPI1_SCLK_PORTP1_MUX | SPI1_MOSI_PORTP1_MUX | SPI1_MISO_PORTP1_MUX;
/* added end */

    return 0;
}

