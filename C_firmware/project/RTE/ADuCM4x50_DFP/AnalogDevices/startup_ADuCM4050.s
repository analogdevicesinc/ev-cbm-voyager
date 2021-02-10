;/**************************************************************************//**
; * @file     startup_ADuCM4050.s
; * @brief    CMSIS Cortex-M4 Core Device Startup File for
; *           Device ADuCM4050
; * @version  V3.10
; * @date     23. November 2012
; *
; * @note     Modified January 11 2018 Analog Devices
; *
; ******************************************************************************/
;/* Copyright (c) 2012 ARM LIMITED
;
;   All rights reserved.
;   Redistribution and use in source and binary forms, with or without
;   modification, are permitted provided that the following conditions are met:
;   - Redistributions of source code must retain the above copyright
;     notice, this list of conditions and the following disclaimer.
;   - Redistributions in binary form must reproduce the above copyright
;     notice, this list of conditions and the following disclaimer in the
;     documentation and/or other materials provided with the distribution.
;   - Neither the name of ARM nor the names of its contributors may be used
;     to endorse or promote products derived from this software without
;     specific prior written permission.
;   *
;   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
;   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;   POSSIBILITY OF SUCH DAMAGE.
;   ------------------------------------------
;
;   Portions Copyright (c) 2018 Analog Devices, Inc.
;
;
; The modules in this file are included in the libraries, and may be replaced
; by any user-defined modules that define the PUBLIC symbol _program_start or
; a user defined start symbol.
; To override the cstartup defined in the library, simply add your modified
; version to the workbench project.
;
; The vector table is normally located at address 0.
; When debugging in RAM, it can be located in RAM, aligned to at least 2^6.
; The name "__vector_table" has special meaning for C-SPY:
; it is where the SP start value is found, and the NVIC vector
; table register (VTOR) is initialized to this address if != 0.
;
; Cortex-M version
;

#include <rtos_map/adi_rtos_map.h>

/* ISRAM is enabled by default and can be disabled by using the following macro.
 * ISRAM must be disabled here when using SRAM MODES 2 or 3.
 */ 
/*#define ADI_DISABLE_INSTRUCTION_SRAM */

#ifdef ADI_DISABLE_INSTRUCTION_SRAM
#include <ADuCM4050_def.h>
#endif
	


        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table
        PUBLIC  __vector_table_0x1c
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA

__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler

        DCD     NMI_Handler
        DCD     HardFault_Handler
        DCD     MemManage_Handler
        DCD     BusFault_Handler
        DCD     UsageFault_Handler
__vector_table_0x1c
        DCD     Reserved_Handler
        DCD     Reserved_Handler
        DCD     Reserved_Handler
        DCD     Reserved_Handler
        DCD     SVC_HANDLER
        DCD     DebugMon_Handler
        DCD     Reserved_Handler
        DCD     PENDSV_HANDLER
        DCD     SYSTICK_HANDLER

        ; External Interrupts
        DCD  RTC1_Int_Handler               /* 0  */
        DCD  Ext_Int0_Handler               /* 1  */
        DCD  Ext_Int1_Handler               /* 2  */
        DCD  Ext_Int2_Handler               /* 3  */ 
        DCD  Ext_Int3_Handler               /* 4  */
        DCD  WDog_Tmr_Int_Handler           /* 5  */
        DCD  Vreg_over_Int_Handler          /* 6  */
        DCD  Battery_Voltage_Int_Handler    /* 7  */
        DCD  RTC0_Int_Handler               /* 8  */
        DCD  GPIO_A_Int_Handler             /* 9  */
        DCD  GPIO_B_Int_Handler             /* 10 */
        DCD  GP_Tmr0_Int_Handler            /* 11 */
        DCD  GP_Tmr1_Int_Handler            /* 12 */
        DCD  Flash0_Int_Handler             /* 13 */
        DCD  UART0_Int_Handler              /* 14 */
        DCD  SPI0_Int_Handler               /* 15 */
        DCD  SPI2_Int_Handler               /* 16 */
        DCD  I2C0_Slave_Int_Handler         /* 17 */
        DCD  I2C0_Master_Int_Handler        /* 18 */
        DCD  DMA_Err_Int_Handler            /* 19 */
        DCD  DMA_SPIH_TX_Int_Handler        /* 20 */
        DCD  DMA_SPIH_RX_Int_Handler        /* 21 */ 
        DCD  DMA_SPORT0A_Int_Handler        /* 22 */
        DCD  DMA_SPORT0B_Int_Handler        /* 23 */
        DCD  DMA_SPI0_TX_Int_Handler        /* 24 */
        DCD  DMA_SPI0_RX_Int_Handler        /* 25 */
        DCD  DMA_SPI1_TX_Int_Handler        /* 26 */
        DCD  DMA_SPI1_RX_Int_Handler        /* 27 */
        DCD  DMA_UART0_TX_Int_Handler       /* 28 */
        DCD  DMA_UART0_RX_Int_Handler       /* 29 */
        DCD  DMA_I2C0_STX_Int_Handler       /* 30 */
        DCD  DMA_I2C0_SRX_Int_Handler       /* 31 */
        DCD  DMA_I2C0_MX_Int_Handler        /* 32 */
        DCD  DMA_AES0_IN_Int_Handler        /* 33 */
        DCD  DMA_AES0_OUT_Int_Handler       /* 34 */
        DCD  DMA_FLASH0_Int_Handler         /* 35 */
        DCD  SPORT0A_Int_Handler            /* 36 */
        DCD  SPORT0B_Int_Handler            /* 37 */
        DCD  Crypto_Int_Handler             /* 38 */
        DCD  DMA_ADC0_Int_Handler           /* 39 */    
        DCD  GP_Tmr2_Int_Handler            /* 40 */
        DCD  Crystal_osc_Int_Handler        /* 41 */
        DCD  SPI1_Int_Handler               /* 42 */
        DCD  PLL_Int_Handler                /* 43 */
        DCD  RNG_Int_Handler                /* 44 */
        DCD  Beep_Int_Handler               /* 45 */    
        DCD  ADC0_Int_Handler               /* 46 */
        DCD  Reserved_Handler               /* 47 */
        DCD  Reserved_Handler               /* 48 */
        DCD  Reserved_Handler               /* 49 */
        DCD  Reserved_Handler               /* 50 */
        DCD  Reserved_Handler               /* 51 */
        DCD  Reserved_Handler               /* 52 */
        DCD  Reserved_Handler               /* 53 */
        DCD  Reserved_Handler               /* 54 */
        DCD  Reserved_Handler               /* 55 */    
        DCD  DMA_SIP0_Int_Handler           /* 56 */
        DCD  DMA_SIP1_Int_Handler           /* 57 */
        DCD  DMA_SIP2_Int_Handler           /* 58 */
        DCD  DMA_SIP3_Int_Handler           /* 59 */
        DCD  DMA_SIP4_Int_Handler           /* 60 */
        DCD  DMA_SIP5_Int_Handler           /* 61 */
        DCD  DMA_SIP6_Int_Handler           /* 62 */
        DCD  DMA_SIP7_Int_Handler           /* 63 */
        DCD  Reserved_Handler               /* 64 */
        DCD  Reserved_Handler               /* 65 */
        DCD  UART1_Int_Handler              /* 66 */
        DCD  DMA_UART1_TX_Int_Handler       /* 67 */
        DCD  DMA_UART1_RX_Int_Handler       /* 68 */
        DCD  RGB_Tmr_Int_Handler            /* 69 */
        DCD  Reserved_Handler               /* 70 */        
        DCD  Root_Clk_Err_Handler           /* 71 */
__Vectors_End

__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB

        PUBWEAK Reset_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler:

#ifdef ADI_DISABLE_INSTRUCTION_SRAM
        /* Disable ISRAM before we access the stack or any data */
        
        /* Load the SRAM control register */
        LDR R1, =REG_PMG0_TST_SRAM_CTL;             
        LDR R0, [R1] ;
        /* Clear the ISRAM enable bit */
        BIC R0, R0, # BITM_PMG_TST_SRAM_CTL_INSTREN ; 
        /* Store ->  ISRAM memory can now be used as data. */ 
        STR R0, [R1] ;                             
#endif
        /* Reset the main stack pointer from the first entry in the 
           vector table (First entry contains the stack pointer) */
        LDR     R0, =__vector_table
        LDR     R0, [R0, #0]
        MSR     MSP, R0
        
        /* Call system init */
        LDR     R0, =SystemInit
        BLX     R0
     
        /* Hand over to IAR */
        LDR     R0, =__iar_program_start
        BX      R0
        
        /* Infinite loop for trap. We should never reach here */
Reset_Handler_0:
        B       Reset_Handler_0


        PUBWEAK NMI_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler:
        B NMI_Handler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
        B HardFault_Handler

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
MemManage_Handler
        B MemManage_Handler

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
BusFault_Handler
        B BusFault_Handler

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UsageFault_Handler
        B UsageFault_Handler

        PUBWEAK SVC_HANDLER
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_HANDLER
        B SVC_HANDLER

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DebugMon_Handler
        B DebugMon_Handler

        PUBWEAK PENDSV_HANDLER
        SECTION .text:CODE:REORDER:NOROOT(1)
PENDSV_HANDLER
        B PENDSV_HANDLER

        PUBWEAK SYSTICK_HANDLER
        SECTION .text:CODE:REORDER:NOROOT(1)
SYSTICK_HANDLER
        B SYSTICK_HANDLER

/****************************************************************
       External interrupts 
*****************************************************************/
        
        PUBWEAK RTC1_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
RTC1_Int_Handler
        B RTC1_Int_Handler

        PUBWEAK Ext_Int0_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int0_Handler
        B Ext_Int0_Handler

        PUBWEAK Ext_Int1_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int1_Handler
        B Ext_Int1_Handler

        PUBWEAK Ext_Int2_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int2_Handler
        B Ext_Int2_Handler

        PUBWEAK Ext_Int3_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int3_Handler
        B Ext_Int3_Handler
            
        PUBWEAK WDog_Tmr_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
WDog_Tmr_Int_Handler
        B WDog_Tmr_Int_Handler    

        PUBWEAK Vreg_over_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Vreg_over_Int_Handler
        B Vreg_over_Int_Handler    

        PUBWEAK Battery_Voltage_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Battery_Voltage_Int_Handler
        B Battery_Voltage_Int_Handler    
        
        PUBWEAK RTC0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
RTC0_Int_Handler
        B RTC0_Int_Handler         
       
        PUBWEAK GPIO_A_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
GPIO_A_Int_Handler
        B GPIO_A_Int_Handler           
       
        PUBWEAK GPIO_B_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
GPIO_B_Int_Handler
        B GPIO_B_Int_Handler         
 
        PUBWEAK GP_Tmr0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
GP_Tmr0_Int_Handler
        B GP_Tmr0_Int_Handler               
 
        PUBWEAK GP_Tmr1_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
GP_Tmr1_Int_Handler
        B GP_Tmr1_Int_Handler   

        PUBWEAK Flash0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Flash0_Int_Handler
        B Flash0_Int_Handler 

        PUBWEAK UART0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART0_Int_Handler
        B UART0_Int_Handler

        PUBWEAK SPI0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI0_Int_Handler
        B SPI0_Int_Handler
          
        PUBWEAK SPI2_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI2_Int_Handler
        B SPI2_Int_Handler           

        PUBWEAK I2C0_Slave_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C0_Slave_Int_Handler
        B I2C0_Slave_Int_Handler    
            
        PUBWEAK I2C0_Master_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C0_Master_Int_Handler
        B I2C0_Master_Int_Handler   

        PUBWEAK DMA_Err_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_Err_Int_Handler
        B DMA_Err_Int_Handler   

        PUBWEAK DMA_SPIH_TX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPIH_TX_Int_Handler
        B DMA_SPIH_TX_Int_Handler  
     
        PUBWEAK DMA_SPIH_RX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPIH_RX_Int_Handler
        B DMA_SPIH_RX_Int_Handler           

        PUBWEAK DMA_SPORT0A_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPORT0A_Int_Handler
        B DMA_SPORT0A_Int_Handler 

        PUBWEAK DMA_SPORT0B_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPORT0B_Int_Handler
        B DMA_SPORT0B_Int_Handler 

        PUBWEAK DMA_SPI0_TX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPI0_TX_Int_Handler
        B DMA_SPI0_TX_Int_Handler 

        PUBWEAK DMA_SPI0_RX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPI0_RX_Int_Handler
        B DMA_SPI0_RX_Int_Handler 
     
        PUBWEAK DMA_SPI1_TX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPI1_TX_Int_Handler
        B DMA_SPI1_TX_Int_Handler 

        PUBWEAK DMA_SPI1_RX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SPI1_RX_Int_Handler
        B DMA_SPI1_RX_Int_Handler


        PUBWEAK DMA_UART0_TX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_UART0_TX_Int_Handler
        B DMA_UART0_TX_Int_Handler
     
        PUBWEAK DMA_UART0_RX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_UART0_RX_Int_Handler
        B DMA_UART0_RX_Int_Handler

        PUBWEAK DMA_I2C0_STX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_I2C0_STX_Int_Handler
        B DMA_I2C0_STX_Int_Handler

        PUBWEAK DMA_I2C0_SRX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_I2C0_SRX_Int_Handler
        B DMA_I2C0_SRX_Int_Handler

        PUBWEAK DMA_I2C0_MX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_I2C0_MX_Int_Handler
        B DMA_I2C0_MX_Int_Handler
    
        PUBWEAK DMA_AES0_IN_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_AES0_IN_Int_Handler
        B DMA_AES0_IN_Int_Handler
    
        PUBWEAK DMA_AES0_OUT_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_AES0_OUT_Int_Handler
        B DMA_AES0_OUT_Int_Handler
         
        PUBWEAK DMA_FLASH0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_FLASH0_Int_Handler
        B DMA_FLASH0_Int_Handler
     
        PUBWEAK SPORT0A_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPORT0A_Int_Handler
        B SPORT0A_Int_Handler
    
        PUBWEAK SPORT0B_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPORT0B_Int_Handler
        B SPORT0B_Int_Handler
      
        PUBWEAK Crypto_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Crypto_Int_Handler
        B Crypto_Int_Handler
         
        PUBWEAK DMA_ADC0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_ADC0_Int_Handler
        B DMA_ADC0_Int_Handler
         
        PUBWEAK GP_Tmr2_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
GP_Tmr2_Int_Handler
        B GP_Tmr2_Int_Handler
          
        PUBWEAK Crystal_osc_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Crystal_osc_Int_Handler
        B Crystal_osc_Int_Handler
        
        PUBWEAK SPI1_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI1_Int_Handler
        B SPI1_Int_Handler
         
        PUBWEAK PLL_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
PLL_Int_Handler
        B PLL_Int_Handler
     
        PUBWEAK RNG_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
RNG_Int_Handler
        B RNG_Int_Handler
            
        PUBWEAK Beep_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Beep_Int_Handler
        B Beep_Int_Handler

        PUBWEAK ADC0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
ADC0_Int_Handler
        B ADC0_Int_Handler

        PUBWEAK DMA_SIP0_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP0_Int_Handler
        B DMA_SIP0_Int_Handler

        PUBWEAK DMA_SIP1_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP1_Int_Handler
        B DMA_SIP1_Int_Handler

        PUBWEAK DMA_SIP2_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP2_Int_Handler
        B DMA_SIP2_Int_Handler

        PUBWEAK DMA_SIP3_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP3_Int_Handler
        B DMA_SIP3_Int_Handler

        PUBWEAK DMA_SIP4_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP4_Int_Handler
        B DMA_SIP4_Int_Handler

        PUBWEAK DMA_SIP5_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP5_Int_Handler
        B DMA_SIP5_Int_Handler

        PUBWEAK DMA_SIP6_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP6_Int_Handler
        B DMA_SIP6_Int_Handler

        PUBWEAK DMA_SIP7_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_SIP7_Int_Handler
        B DMA_SIP7_Int_Handler

        PUBWEAK Ext_Int4_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int4_Handler
        B Ext_Int4_Handler

        PUBWEAK Ext_Int5_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Ext_Int5_Handler
        B Ext_Int5_Handler

        PUBWEAK UART1_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART1_Int_Handler
        B UART1_Int_Handler
            
        PUBWEAK DMA_UART1_TX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_UART1_TX_Int_Handler
        B DMA_UART1_TX_Int_Handler
            
        PUBWEAK DMA_UART1_RX_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA_UART1_RX_Int_Handler
        B DMA_UART1_RX_Int_Handler

        PUBWEAK RGB_Tmr_Int_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
RGB_Tmr_Int_Handler
        B RGB_Tmr_Int_Handler

        PUBWEAK Root_Clk_Err_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Root_Clk_Err_Handler
        B Root_Clk_Err_Handler      

        PUBWEAK Reserved_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
Reserved_Handler
        B Reserved_Handler        
        
        END
