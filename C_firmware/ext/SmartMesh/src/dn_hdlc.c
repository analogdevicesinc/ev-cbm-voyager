/*
Copyright (c) 2014, Dust Networks. All rights reserved.

HDLC library.

\license See attached DN_LICENSE.txt.
*/

#include "dn_hdlc.h"
#include "dn_uart.h"
#include "dn_lock.h"

//=========================== variables =======================================

typedef struct {
   // admin
   dn_hdlc_rxFrame_cbt  rxFrame_cb;
   // input
   uint8_t              lastRxByte;
   bool                 busyReceiving;
   bool                 inputEscaping;
   uint16_t             inputCrc;
   uint8_t              inputBufFill;
   uint8_t              inputBuf[DN_HDLC_INPUT_BUFFER_SIZE];
   // output
   uint16_t             outputCrc;
} dn_hdlc_vars_t;

dn_hdlc_vars_t dn_hdlc_vars;

//=========================== prototypes ======================================

// callback handlers
void dn_hdlc_rxByte(uint8_t rxbyte);
// input
void dn_hdlc_inputOpen();
void dn_hdlc_inputWrite(uint8_t b);
void dn_hdlc_inputClose();
// helpers
uint16_t dn_hdlc_crcIteration(uint16_t crc, uint8_t data_byte);

//=========================== public ==========================================

/**
\brief Setting up the instance.
*/
void dn_hdlc_init(dn_hdlc_rxFrame_cbt rxFrame_cb) {
   // reset local variables
   memset(&dn_hdlc_vars,   0, sizeof(dn_hdlc_vars));
   
   // store params
   dn_hdlc_vars.rxFrame_cb = rxFrame_cb;
   
   // initialize UART
   dn_uart_init(dn_hdlc_rxByte);
}

//=========================== private =========================================

//===== callback_handler

/**
\brief Function which getted called each time a byte is received over UART.

\param[in] rxbyte The received byte.
*/
void dn_hdlc_rxByte(uint8_t rxbyte) {
   
   // lock the module
   dn_lock();
   
   if        (
         dn_hdlc_vars.busyReceiving==FALSE  &&
         dn_hdlc_vars.lastRxByte==DN_HDLC_FLAG &&
         rxbyte!=DN_HDLC_FLAG
      ) {
      // start of frame
      
      // I'm now receiving
      dn_hdlc_vars.busyReceiving       = TRUE;
      
      // create the HDLC frame
      dn_hdlc_inputOpen();
      
      // add the byte just received
      dn_hdlc_inputWrite(rxbyte);
   } else if (
         dn_hdlc_vars.busyReceiving==TRUE   &&
         rxbyte!=DN_HDLC_FLAG
      ){
      // middle of frame
      
      // add the byte just received
      dn_hdlc_inputWrite(rxbyte);
      
      if (dn_hdlc_vars.inputBufFill+1>DN_HDLC_INPUT_BUFFER_SIZE) {
         // input buffer overflow
         dn_hdlc_vars.inputBufFill       = 0;
         dn_hdlc_vars.busyReceiving      = FALSE;
      }
   } else if (
         dn_hdlc_vars.busyReceiving==TRUE   &&
         rxbyte==DN_HDLC_FLAG
      ) {
      // end of frame
      
      // finalize the HDLC frame
      dn_hdlc_inputClose();
      
      if (dn_hdlc_vars.inputBufFill==0) {
         // invalid HDLC frame
      } else {
         // hand over frame to upper layer
         dn_hdlc_vars.rxFrame_cb(&dn_hdlc_vars.inputBuf[0],dn_hdlc_vars.inputBufFill);
         
         // clear inputBuffer
         dn_hdlc_vars.inputBufFill=0;
      }
      
      dn_hdlc_vars.busyReceiving = FALSE;
   }
   
   dn_hdlc_vars.lastRxByte = rxbyte;
   
   // unlock the module
   dn_unlock();
}

//===== output

/**
\brief Start an HDLC frame in the output buffer.
*/
void dn_hdlc_outputOpen() {
   // initialize the value of the CRC
   dn_hdlc_vars.outputCrc = DN_HDLC_CRCINIT;
   
   // send opening HDLC flag
   dn_uart_txByte(DN_HDLC_FLAG);
}

/**
\brief Add a byte to the outgoing HDLC frame being built.

\param[in] b The byte to add.
*/
void dn_hdlc_outputWrite(uint8_t b) {
   
   // iterate through CRC calculator
   dn_hdlc_vars.outputCrc = dn_hdlc_crcIteration(dn_hdlc_vars.outputCrc,b);
   
   // write optional escape byte
   if (b==DN_HDLC_FLAG || b==DN_HDLC_ESCAPE) {
      dn_uart_txByte(DN_HDLC_ESCAPE);
      b = b^DN_HDLC_ESCAPE_MASK;
   }
   
   // data byte
   dn_uart_txByte(b);
}

/**
\brief Finalize the outgoing HDLC frame.
*/
void dn_hdlc_outputClose() {
   uint16_t   finalCrc;
   
   // finalize the calculation of the CRC
   finalCrc   = ~dn_hdlc_vars.outputCrc;
   
   // write the CRC value
   dn_hdlc_outputWrite((finalCrc>>0)&0xff);
   dn_hdlc_outputWrite((finalCrc>>8)&0xff);
   
   // write closing HDLC flag
   dn_uart_txByte(DN_HDLC_FLAG);
   
   // flush the UART
   dn_uart_txFlush();
}

//===== input

/**
\brief Start an HDLC frame in the input buffer.
*/
void dn_hdlc_inputOpen() {
   // reset the input buffer index
   dn_hdlc_vars.inputBufFill = 0;
   
   // initialize the value of the CRC
   dn_hdlc_vars.inputCrc = DN_HDLC_CRCINIT;
}

/**
\brief Add a byte to the incoming HDLC frame.

\param[in] b The byte to add.
*/
void dn_hdlc_inputWrite(uint8_t b) {
   if (b==DN_HDLC_ESCAPE) {
      dn_hdlc_vars.inputEscaping = TRUE;
   } else {
      if (dn_hdlc_vars.inputEscaping==TRUE) {
         b = b^DN_HDLC_ESCAPE_MASK;
         dn_hdlc_vars.inputEscaping = FALSE;
      }
      
      // add byte to input buffer
      dn_hdlc_vars.inputBuf[dn_hdlc_vars.inputBufFill] = b;
      dn_hdlc_vars.inputBufFill++;
      
      // iterate through CRC calculator
      dn_hdlc_vars.inputCrc = dn_hdlc_crcIteration(dn_hdlc_vars.inputCrc,b);
   }
}

/**
\brief Finalize the incoming HDLC frame.
*/
void dn_hdlc_inputClose() {
   
   // verify the validity of the frame
   if (dn_hdlc_vars.inputCrc==DN_HDLC_CRCGOOD) {
      // the CRC is correct
      
      // remove the CRC from the input buffer
      dn_hdlc_vars.inputBufFill    -= 2;
   } else {
      // the CRC is incorrect
      
      // drop the incoming fram
      dn_hdlc_vars.inputBufFill     = 0;
   }
   
   // reset escaping
   dn_hdlc_vars.inputEscaping = FALSE;
}

//=========================== helpers =========================================

/**
\brief Perform a single CRC iteration.

\param[in] crc The current running CRC value.
\param[in] b   The new byte.

\return The updated CRC running value.
*/
uint16_t dn_hdlc_crcIteration(uint16_t crc, uint8_t b) {
   return (crc >> 8) ^ dn_hdlc_fcstab[(crc ^ b) & 0xff];
}
