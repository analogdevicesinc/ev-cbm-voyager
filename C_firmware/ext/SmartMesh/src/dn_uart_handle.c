/*=========================== Defines =========================================*/
#define Rx_buffer_size   1
#define Tx_buffer_size   1
#define DMA_mode         0 
#define Rx_Buffer_Full   0x05
#define Div_C            4
#define Div_M            1
#define Div_N            1563
#define Uart_OSR         3

/*
  Table 21-2: Baud Rate Examples Based on 26 MHz PCLK
  Baud Rate OSR COMDIV DIVM DIVN
  9600        3 24     3    1078
  19200       3 12     3    1078
  38400       3 8      2    1321
  57600       3 4      3    1078
  115200      3 4      1    1563
  230400      3 2      1    1563
  460800      3 1      1    1563
  921,600     2 1      1    1563
  1,000,000   2 1      1    1280
  1,500,000   2 1      1    171

These are calculated with the UarDivCalculator tool.
*/

/*=========================== Includes =========================================*/
#include <stdio.h>
#include <drivers/uart/adi_uart.h>
#include <drivers/i2c/adi_i2c.h>
#include "common.h"
#include <drivers/pwr/adi_pwr.h>
#include "dn_uart.h"
#include "adi_callback.h"
//#include "SmartMesh_RF_cog.h"
#include "drivers/gpio/adi_gpio.h" 


/* Handle for the UART device. */
static ADI_UART_HANDLE hDevice;

/* Memory for the UART driver. */
static uint8_t UartDeviceMem[ADI_UART_MEMORY_SIZE];
uint32_t pHwError_uart;


/*=========================== Packet Buffer ===================================*/
char inbuf_mote_boot[16];
/*=========================== Variables =======================================*/
typedef struct 
{
   dn_uart_rxByte_cbt   ipmt_uart_rxByte_cb;
} dn_uart_vars_t;

dn_uart_vars_t dn_uart_vars;

/*Call_back submit variable*/
uint8_t *Rx_buff;

/*Buffer variables*/
extern int head,tail,next,max_len;
extern uint8_t  buffer_uart[buffer_size];
extern bool full,empty;

/* data array statics for temp sensor */
uint8_t rxData[DATASIZE_i2c];

/* device memory */
uint8_t devMem[ADI_I2C_MEMORY_SIZE];

/* device handle */
ADI_I2C_HANDLE i2cDevice;

/* addressing/command phase "prologue" array */
uint8_t prologueData[5];

/* transaction structure */
ADI_I2C_TRANSACTION xfr;

/* result */
ADI_I2C_RESULT result = ADI_I2C_SUCCESS;

/* HW Error result */
uint32_t hwError_i2c;

/*=========================== Flush ===========================================*/  

void dn_uart_txFlush()
{
  /*noting here*/
}
/*=========================== Receive ISR  ====================================*/                  
/* SmartMesh_RF_cog_receive_ISR handles the interrupt for Rx_buffer full and places the incoming data in a buffer,byte oriented receive implemented */

void SmartMesh_RF_cog_receive_ISR(void *pcbParam,uint32_t Event,void *pArg)
{ 
  next=head+1;

  if(next>=max_len)
     next=0;
  if(next==tail)
     full=true;
  else
  { 
#ifdef COG
       /*COG Board Hardware*/
       buffer_uart[head]=*pREG_UART0_RX;
#else  
       /*CMG MVP Hardware*/
       buffer_uart[head]=*pREG_UART1_RX;   
       //adi_uart_Read(hDevice, Rx_buff, 1, DMA_mode, &pHwError_uart);
       //buffer_uart[head]=*Rx_buff;
#endif
       head=next;
  }
}

/*=============================Buffer handle ==================================*/
void Smartmesh_RF_cog_receive(void)
{
  dn_uart_vars.ipmt_uart_rxByte_cb(buffer_uart[tail]);                          /* This function is called from the application level */
}

/*=========================== Uart Initialization  ============================*/

/* dn_uart_init  Initialises the uart and registers for the Rx buffer full call back */
void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb)
{

   /*call back function*/
   dn_uart_vars.ipmt_uart_rxByte_cb = rxByte_cb;
  
   /*enabling RF reset*/
   adi_gpio_OutputEnable( RESET_SM_PORT, RESET_SM_PIN, true); 
   
   /*RESET the SmartMesh Mote*/
   adi_gpio_SetHigh( RESET_SM_PORT, RESET_SM_PIN); 
   for(int i=0;i<10000;i++)
     asm("nop");      
   adi_gpio_SetLow( RESET_SM_PORT, RESET_SM_PIN);  
   for(int i=0;i<10000;i++)
     asm("nop");
   adi_gpio_SetHigh( RESET_SM_PORT, RESET_SM_PIN); 
   
   /* configure UART */     
   /* Open the bidirectional UART device. */
   adi_uart_Open(UART_DEVICE_NUM, ADI_UART_DIR_BIDIRECTION, UartDeviceMem, ADI_UART_MEMORY_SIZE, &hDevice);
        

   /* Buad rate initialization. */
   adi_uart_ConfigBaudRate(hDevice,Div_C,Div_M,Div_N,Uart_OSR);                 /* 15200 baud rate */
  
   
/*===========================I2C initialisation================================*/  
   /*Comment out, not using the temp sensor*/
#if 0
   /* enabling gpio for i2c */
   adi_gpio_OutputEnable( ADI_GPIO_PORT1, ADI_GPIO_PIN_12, true); 
   adi_gpio_SetHigh( ADI_GPIO_PORT1, ADI_GPIO_PIN_12); 
    
   adi_gpio_OutputEnable( ADI_GPIO_PORT2, ADI_GPIO_PIN_2, true); 
   adi_gpio_SetLow( ADI_GPIO_PORT2, ADI_GPIO_PIN_2); 
    
   adi_i2c_Open(0, &devMem, ADI_I2C_MEMORY_SIZE, &i2cDevice);   /* I2c */
   adi_i2c_Reset(i2cDevice); 
   adi_i2c_SetBitRate(i2cDevice, 400000);
   adi_i2c_SetSlaveAddress(i2cDevice, 0x48);
       
   prologueData[0]     = 0x0b;                                  /* address of ID register */
   xfr.pPrologue       = &prologueData[0];
   xfr.nPrologueSize   = 1;
   xfr.pData           = rxData;
   xfr.nDataSize       = 1;
   xfr.bReadNotWrite   = true;
   xfr.bRepeatStart    = true;

   /* clear chip ID readback value in receive buffer */
   rxData[0] = 0;

   /* blocking read */
   adi_i2c_ReadWrite(i2cDevice, &xfr, &hwError_i2c);    
#endif

   
/*=========================== Debug_wait=======================================*/

   adi_uart_Read(hDevice,inbuf_mote_boot,16,0,&pHwError_uart);                       /* mote boot event */ 
   
/*=============================================================================*/

   /*Buffer Variable initialisation*/
   head = 0;
   tail = 0;
   max_len = buffer_size;
   next = 0;
   empty = false;
   full = false;
        
   /*Register for call back and call back buffer submission*/
   dn_uart_irq_enable();

   adi_uart_RegisterCallback(hDevice, SmartMesh_RF_cog_receive_ISR, NULL);      /* Registering call back */
   adi_uart_SubmitRxBuffer(hDevice, Rx_buff, Rx_buffer_size, DMA_mode);  
}        

void dn_uart_irq_enable(void) 
{
#ifdef COG
   /*COG board Hardware*/
   *pREG_UART0_IEN = Rx_Buffer_Full;                                            /* enabling receive buffer full interrupt */
   Rx_buff=(uint8_t*)pREG_UART0_RX;  
#else
   /*CMG MVP Hardware*/
   *pREG_UART1_IEN = Rx_Buffer_Full;                                            /* enabling receive buffer full interrupt */
   Rx_buff=(uint8_t*)pREG_UART1_RX;
#endif
}

void dn_uart_irq_disable(void) 
{
#ifdef COG
   /*COG board Hardware*/
   *pREG_UART0_IEN = *pREG_UART0_IEN & ~Rx_Buffer_Full;            /* disabling receive buffer full interrupt */
#else
   /*CMG MVP Hardware*/
   *pREG_UART1_IEN = *pREG_UART1_IEN & ~Rx_Buffer_Full;            /* disabling receive buffer full interrupt */
#endif
}

/*=========================== Transmission ========================================*/

/*dn_uart_txByte is called by the C-library for transmission of packets,byte oriented transfer is implemented */

   void dn_uart_txByte(uint8_t byte)
{ 
       adi_uart_Write(hDevice, &byte, Tx_buffer_size, DMA_mode, &pHwError_uart);     /* Transmission of a byte done here */
}
 
