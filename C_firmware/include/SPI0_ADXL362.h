/*********************************************************************************
Copyright(c) 2018-2020 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors. 
By using this software you agree to the terms of the associated Analog Devices 
License Agreement.
*********************************************************************************/
/*
 * @file      ADC_channel_read.h
 * @brief     Example to demonstrate ADC driver - modified for ADuCm4050
 * @details
 *            This is the primary include file for the ADC example.
 *
 */

/*=============  I N C L U D E S   =============*/

/* ADC Driver includes */
#include <drivers/spi/adi_spi.h>
#include <drivers/dma/adi_dma.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*=============  D E F I N E S  =============*/

/* Write samples to file */
//#ifndef __CC_ARM
//#define WRITE_SAMPLES_TO_FILE
//#endif

/* SPI Device number */
#define BUFFERSIZE                256u   
#define SPI_MASTER_DEVICE_NUM     (0u) 
//extern uint16_t adcData[ADC_DATA_LEN];

/* --- command --- */
#define XL362_REG_WRITE           0x0a
#define XL362_REG_READ            0x0b
#define XL362_FIFO_READ           0x0d

/* ------- Register names ------- */
/* -- part ID constants -- */
#define XL362_DEVID_AD            0x00
#define XL362_DEVID_MST           0x01
#define XL362_PARTID              0x02
#define XL362_REVID               0x03
#define XL362_XID                 0x04

/* -- RO data -- */
#define XL362_XDATA8              0x08
#define XL362_YDATA8              0x09
#define XL362_ZDATA8              0x0A
#define XL362_STATUS              0x0B
#define XL362_FIFO_ENTRIES_L      0x0C
#define XL362_FIFO_ENTRIES_H      0x0D
#define XL362_XDATAL              0x0e
#define XL362_XDATAH              0x0f
#define XL362_YDATAL              0x10
#define XL362_YDATAH              0x11
#define XL362_ZDATAL              0x12
#define XL362_ZDATAH              0x13
#define XL362_TEMPL               0x14
#define XL362_TEMPH               0x15
#define XL362_X_ADCL              0x16
#define XL362_X_ADCH              0x17

/* -- Control and Config -- */
#define XL362_SOFT_RESET          0x1f
#define XL362_THRESH_ACTL         0x20
#define XL362_THRESH_ACTH         0x21
#define XL362_TIME_ACT            0x22
#define XL362_THRESH_INACTL       0x23
#define XL362_THRESH_INACTH       0x24
#define XL362_TIME_INACTL         0x25
#define XL362_TIME_INACTH         0x26
#define XL362_ACT_INACT_CTL       0x27
#define XL362_FIFO_CONTROL        0x28
#define XL362_FIFO_SAMPLES        0x29
#define XL362_INTMAP1             0x2a
#define XL362_INTMAP2             0x2b
#define XL362_FILTER_CTL          0x2c
#define XL362_POWER_CTL           0x2d
#define XL362_SELF_TEST           0x2e

/*----------------------------------------------------------------------
  Bit field definitions and register values
  ----------------------------------------------------------------------*/
//#define XL362_
/* Key Value in soft reset                                              */
#define XL362_SOFT_RESET_KEY      0x52

/* Registers THRESH_ACTL through TIME_INACTH just take values
   in lsbs or samples.
   There are no specific bit fields in these registers                  */
/* Bit values in ACT_INACT_CTL  (6 bits allocated)                      */
#define XL362_ACT_ENABLE          0x01
#define XL362_ACT_DISABLE         0x00
#define XL362_ACT_AC              0x02
#define XL362_ACT_DC              0x00
#define XL362_INACT_ENABLE        0x04
#define XL362_INACT_DISABLE       0x00
#define XL362_INACT_AC            0x08
#define XL362_INACT_DC            0x00
#define XL362_ACT_INACT_LINK      0x10
#define XL362_ACT_INACT_LOOP      0x20

/* Bit values in FIFO_CTL  (4 bits allocated)                            */
#define XL362_FIFO_MODE_OFF       0x00
#define XL362_FIFO_MODE_FIFO      0x01
#define XL362_FIFO_MODE_STREAM    0x02
#define XL362_FIFO_MODE_TRIGGER   0x03
#define XL362_FIFO_TEMP           0x04
#define XL362_FIFO_SAMPLES_AH     0x08



/* bit values in INTMAP1 and INTMAP2 (and status)                        */
#define XL362_INT_DATA_READY      0x01
#define XL362_INT_FIFO_READY      0x02
#define XL362_INT_FIFO_WATERMARK  0x04
#define XL362_INT_FIFO_OVERRUN    0x08
#define XL362_INT_ACT             0x10
#define XL362_INT_INACT           0x20
#define XL362_INT_AWAKE           0x40
#define XL362_INT_LOW             0x80

/* Bit values in FILTER_CTL  (8 bits allocated)                            */
#define XL362_RATE_400            0x05
#define XL362_RATE_200            0x04
#define XL362_RATE_100            0x03  /* default */
#define XL362_RATE_50             0x02
#define XL362_RATE_25             0x01
#define XL362_RATE_12_5           0x00

#define XL362_EXT_TRIGGER         0x08

#define XL362_AXIS_X              0x00
#define XL362_AXIS_Y              0x10
#define XL362_AXIS_Z              0x20

#define XL362_RANGE_2G            0x00
#define XL362_RANGE_4G            0x40
#define XL362_RANGE_8G            0x80


/* Bit values in POWER_CTL  (8 bits allocated)                            */
#define XL362_STANDBY             0x00
#define XL362_MEASURE_1D          0x01
#define XL362_MEASURE_3D          0x02
#define XL362_AUTO_SLEEP          0x04
#define XL362_SLEEP               0x08

#define XL362_LOW_POWER           0x00
#define XL362_LOW_NOISE1          0x10
#define XL362_LOW_NOISE2          0x20
#define XL362_LOW_NOISE3          0x30

#define XL362_EXT_CLOCK           0x40
#define XL362_EXT_ADC             0x80

/* Bit values in SELF_TEST  (1 bit allocated)                            */
#define XL362_SELFTEST_ON         0x01
#define XL362_SELFTEST_OFF        0x00


/*****/
/*=============  PROTOTYPES  =============*/
void xl362init(void); 
void xl362_SampleData_Blocking(uint8_t extra_bits, uint8_t samp_time);
void xl362disable(void);

/* Wrapper functions for reading and writing bursts to / from the XL362
   Will need to be modified for your hardware 
*/

/*
  The read function takes a byte count, a register address and a
  pointer to the buffer where to return the data.  When the read
  function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    3) Send the register address
    4) Read each byte
    5) CS_N high
*/
void xl362Read(unsigned char count, unsigned char regaddr, unsigned char *buf);
/*
  The fifo read function takes a byte count as an int and a
  pointer to the buffer where to return the data.  When the read
  function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    4) Read each byte
    5) CS_N high
*/
void xl362FifoRead(unsigned int count, unsigned char *buf);
/*
  The write function takes a byte count, and a pointer to the buffer
  with the data.  The first byte of the data should be the start
  register address, the remaining bytes will be written starting at
  that register.  The mininum bytecount that shoudl be passed is 2,
  one byte of address, followed by a byte of data.  Multiple
  sequential registers can be written with longer byte counts. When
  the write function runs, it goes through the following sequence:

    1) CS_N Low
    2) Send the correct command
    3) Send the register address
    4) Send each byte
    5) CS_N high
*/
void xl362Write(unsigned char count, unsigned char regaddr, unsigned char *buf);