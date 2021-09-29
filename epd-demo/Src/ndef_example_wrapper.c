/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT 2019 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        www.st.com/myliberty
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/
/*! \file
 *
 *  \author 
 *
 *  \brief Demo application
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to exchange data with these devices, using the RFAL library.
 *
 *  This demo does not fully implement the activities according to the standards,
 *  it performs the required to communicate with a card/device and retrieve 
 *  its UID. Also blocking methods are used for data exchange which may lead to
 *  long periods of blocking CPU/MCU.
 *  For standard compliant example please refer to the Examples provided
 *  with the RFAL library.
 * 
 */
 
/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "demo.h"

/*! NDEF read usage example */
extern void ndefExampleRead(void);

/*! NDEF write usage example */
extern void ndefExampleWrite(void);

/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static bool bWrite = false;

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 * \return true once initialized
 *
 *  This function Initializes the required layers for the demo
 *****************************************************************************
 */
bool demoIni( void )
{
    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        bWrite = !bWrite;
        platformLog("Write: %s\r\n", bWrite ? "ON": "OFF");
        /* Debounce button */
        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
    }
    return (true);
}

/*!
 *****************************************************************************
 * \brief Demo Cycle
 *
 *  This function executes the demo state machine. 
 *  It must be called periodically
 *****************************************************************************
 */
void demoCycle( void )
{   
    if( !bWrite )
    {
        ndefExampleRead();
    }
    else 
    {
        ndefExampleWrite();
    }
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
