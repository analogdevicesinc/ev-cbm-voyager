/*!
 *****************************************************************************
   @file:    adi_adxl363_config.h
   @brief:   Device Configuration options for adxl363 Accelerometer.
  -----------------------------------------------------------------------------

Copyright (c) 2016 Analog Devices, Inc.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
  - Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  - Modified versions of the software must be conspicuously marked as such.
  - This software is licensed solely and exclusively for use with processors
    manufactured by or for Analog Devices, Inc.
  - This software may not be combined or merged with other code in any manner
    that would cause the software to become subject to terms and conditions
    which differ from those listed here.
  - Neither the name of Analog Devices, Inc. nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.
  - The use of this software may or may not infringe the patent rights of one
    or more patent holders.  This license does not release you from the
    requirement that you obtain separate licenses from these patent holders
    to use this software.

THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
TITLE, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
NO EVENT SHALL ANALOG DEVICES, INC. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, PUNITIVE OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, DAMAGES ARISING OUT OF CLAIMS OF INTELLECTUAL
PROPERTY RIGHTS INFRINGEMENT; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/

#ifndef ADI_ADXL363_CONFIG_H
#define ADI_ADXL363_CONFIG_H
#include <adi_global_config.h>

/* Used for chip select definitions */
#include <drivers/spi/adi_spi.h>


/** @addtogroup ADXL363_Driver_Config Static Configuration
 *  @ingroup ADXL363_Driver
 *  @{
 */

/*!  SPI device number for use with the ADXLl363. 
*/
#define ADXL363_CFG_SPI_DEV_NUM             2u

/*!  SPI chip select for use with the ADXLl363. 
*/
#define ADXL363_CFG_SPI_DEV_CS              ADI_SPI_CS2

/*************************************************************
                 ADXL363  configurations
 *************************************************************/

/*! To detect activity, the ADXL363 compares the absolute value of the 12-bit (signed) 
  *  acceleration data with the 11-bit (unsigned) activity threshold value. 
  *  Activity is considered to be any movement that breaks this threshold.
  *  Valid Range 1 - 1024  
*/
#define ADXL363_CFG_ACTIVITY_THRESHOLD            0X20

/*! The activity timer implements a robust activity detection that minimizes
  *  false positive motion triggers. The value in this register sets the number 
  *  of consecutive samples that must have at least one axis greater than the 
  *  activity threshold (set by #ADXL363_CFG_ACTIVITY_THRESHOLD) for an 
  *  activity event to be detected.
  *  Valid Range 1-1024
*/
#define ADXL363_CFG_ACTIVITY_TIME                 1

/*! To detect inactivity, the ADXL363 compares the absolute value of the 12-bit (signed) 
  *  acceleration data with the 11-bit (unsigned) inactivity threshold value.
  *  Inactivity is any movement that is below this threshold.
  *  Valid Range 1 - 1024  
*/
#define ADXL363_CFG_INACTIVITY_THRESHOLD          0X20


/*! Sets the number of consecutive samples that must have all axes lower 
  *  than the inactivity threshold (set by ADXL363_CFG_INACTIVITY_THRESHOLD) for 
  *  an inactivity event to be detected. 
  *  Valid Range 1 - 1024   
*/
#define ADXL363_CFG_INACTIVITY_TIME               0X20

/*! 
  *  Enable/disable the activity (overthreshold) detection. It can be set to\n
  *  0 -  Disable activity.\n
  *  1 -  Enable activity.\n
*/
#define ADXL363_CFG_ENABLE_ACTIVITY               1

/*! 
  *  Absolute/reference mode selection for the activity detection.\n
  *  0 -  Activity detection function operates in absolute mode.\n
  *  1 -  Activity detection function operates in referenced mode.\n
*/
#define ADXL363_CFG_ACTIVITY_MODE                 1

/*! 
  *  Enable/disable the inactivity (underthreshold detection). It can be set to\n
  *  0 -  Disable inactivity detection\n
  *  1 -  Enable inactivity detection.\n
*/
#define ADXL363_CFG_ENABLE_INACTIVITY             1

/*! 
  *  Select absolute/reference mode selection for the inactivity detection.\n
  *   0 -  Inactivity detection function operates in absolute mode.\n
  *   1 -  Inactivity detection function operates in referenced mode.\n
*/
#define ADXL363_CFG_INACTIVITY_MODE               1

/*! 
  *  Select Normal/Link/Loop/ mode.\n
  *  0 -  Normal mode of operation.\n
  *  1 -  Link mode of operation.\n
  *  3 -  Loop mode of operation.    \n
*/
#define ADXL363_CFG_LINK_LOOP_MODE                3

/*! 
  *   Select FIFO mode.\n
  *    0 -  Disable the FIFO\n
  *    1 -  Oldest saved mode.\n
  *    3 -  Stream mode.\n
  *    4 -  Triggered mode.\n
*/
#define ADXL363_CFG_FIFO_MODE                     0

/*! 
  *   Enable the inclusion of temperature data in FIFO sampling.\n
  *   0 - Disable the inclusion of temperature data in FIFO \n
  *   1 - Enable the inclusion of temperature data in the FIFO.
*/
#define ADXL363_CFG_ENABLE_TEMPERATURE_FIFO       0

/*! 
  *  Sets the number of  number of samples to be stored in the FIFO.
  *  Valid Range 1 - 511   */
#define ADXL363_CFG_FIFO_SIZE                     0

/*! 
  *  Interrupt mask for the generation of the INT1 interrupt on the ADXL363. 
 */
#define ADXL363_CFG_INT1_MAP                      0x40

/*! 
   *  Interrupt mask for the generation of the INT2 interrupt on the ADXL363.
 */
#define ADXL363_CFG_INT2_MAP                      0x40

/*! 
  *  Select the output data rate\n
  *  0 - 12.5 Hz \n
  *  1 - 25   Hz \n
  *  2 - 50   Hz \n
  *  3 - 100  Hz \n
  *  4 - 200  Hz \n
  *  5 - 400  Hz \n
  (Value of 6 and 7 sets the output data rate to 400 Hz)\n
 */
#define ADXL363_CFG_OUTPUT_DATARATE               3

/*! 
  *  Select the Antialiasing filter bandwidth\n
  *  0 - The bandwidth of the filters is set to half the ODR for more conservative filtering.\n
  *  1 - The bandwidth of the filters is set to quarter the ODR for a wider bandwidth.\n
 */
#define ADXL363_CFG_FILTER_BW                     0

/*! 
  *  Select Measurement range\n
  *  0 - +- 2g\n
  *  1 - +- 4g\n
  *  2 - +- 8g \n
  * ( value of 3 also sets the measurement range to +- 8g )\n
*/
#define ADXL363_CFG_MEASUREMENT_RANGE             0


/*! 
  *  Enable Measurement\n
  *  0  - Standby mode\n
  *  2  - Measurement mode.\n
 */
#define ADXL363_CFG_ENABLE_MEASUREMENT            0

/*! 
  *  Enable Autosleep\n
  *  0  - To disable auto sleep\n
  *  1  - To enable auto sleep\n
 */
#define ADXL363_CFG_ENABLE_AUTOSLEEP              1
/*! 
  *  To enable wakeup  mode\n
  *  0  - To disable wakeup  mode\n
  *  1  - To enable wakeup  mode\n
 */
#define ADXL363_CFG_ENABLE_WAKEUP_MODE            0

/*! 
  *  Select Power vs. Noise\n
  *  0 -  Normal operation\n
  *  1 -  Low noise mode.\n
  *  2 -  Ultralow noise mode\n
 */
#define ADXL363_CFG_NOISE_MODE                    0


/************** Macro validation *****************************/

#if (ADXL363_CFG_ENABLE_ACTIVITY > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ACTIVITY_MODE > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ENABLE_INACTIVITY > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_INACTIVITY_MODE > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_LINK_LOOP_MODE > 3)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_FIFO_MODE > 5)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ENABLE_TEMPERATURE_FIFO > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_FIFO_SIZE > 512)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_OUTPUT_DATARATE > 7)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_FILTER_BW > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_MEASUREMENT_RANGE > 2)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ENABLE_MEASUREMENT > 2)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ENABLE_AUTOSLEEP > 1)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ENABLE_WAKEUP_MODE > 1) 
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_NOISE_MODE > 2)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_INACTIVITY_TIME > 1024)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ACTIVITY_TIME > 1024)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_ACTIVITY_THRESHOLD > 1024)
#error "Invalid configuration"
#endif

#if (ADXL363_CFG_INACTIVITY_THRESHOLD > 1024)
#error "Invalid configuration"
#endif

/**
 *  @}
 */
#endif /* ADI_ADXL363_CONFIG_H */
