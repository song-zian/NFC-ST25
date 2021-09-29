/******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/myliberty
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
 *  \brief Platform header file. Defining platform independent functionality.
 *
 */


/*
 *      PROJECT:   
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */

/*! \file platform.h
 *
 *  \author Gustavo Patricio
 *
 *  \brief Platform specific definition layer  
 *  
 *  This should contain all platform and hardware specifics such as 
 *  GPIO assignment, system resources, locks, IRQs, etc
 *  
 *  Each distinct platform/system/board must provide this definitions 
 *  for all SW layers to use
 *  
 */

#ifndef PLATFORM_H
#define PLATFORM_H

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#include "spi.h"
#include "timer.h"
#include "main.h"
#include "logger.h"


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define ST25R391X_SS_PIN            ST25_CS_PIN_Pin         /*!< GPIO pin used for ST25R3911 SPI SS                */ 
#define ST25R391X_SS_PORT           ST25_CS_PIN_GPIO_Port   /*!< GPIO port used for ST25R3911 SPI SS port          */ 

#define ST25R391X_INT_PIN           ST25_IRQ_PIN_Pin        /*!< GPIO pin used for ST25R3911 External Interrupt    */
#define ST25R391X_INT_PORT          ST25_IRQ_PIN_GPIO_Port  /*!< GPIO port used for ST25R3911 External Interrupt   */

#ifdef LED_FIELD_Pin
#define PLATFORM_LED_FIELD_PIN      LED_FIELD_Pin       /*!< GPIO pin used as field LED                        */
#endif

#ifdef LED_FIELD_GPIO_Port
#define PLATFORM_LED_FIELD_PORT     LED_FIELD_GPIO_Port /*!< GPIO port used as field LED                       */
#endif


#define PLATFORM_LED_A_PIN           GREEN_LED_Pin             /*!< GPIO pin used for LED A    */
#define PLATFORM_LED_A_PORT          GREEN_LED_GPIO_Port       /*!< GPIO port used for LED A   */
#define PLATFORM_LED_B_PIN           GREEN_LED_Pin             /*!< GPIO pin used for LED B    */
#define PLATFORM_LED_B_PORT          GREEN_LED_GPIO_Port       /*!< GPIO port used for LED B   */
#define PLATFORM_LED_F_PIN           GREEN_LED_Pin             /*!< GPIO pin used for LED F    */
#define PLATFORM_LED_F_PORT          GREEN_LED_GPIO_Port       /*!< GPIO port used for LED F   */
#define PLATFORM_LED_V_PIN           GREEN_LED_Pin             /*!< GPIO pin used for LED V    */
#define PLATFORM_LED_V_PORT          GREEN_LED_GPIO_Port       /*!< GPIO port used for LED V   */
#define PLATFORM_LED_AP2P_PIN        GREEN_LED_Pin          /*!< GPIO pin used for LED AP2P */
#define PLATFORM_LED_AP2P_PORT       GREEN_LED_GPIO_Port    /*!< GPIO port used for LED AP2P*/

#define PLATFORM_USER_BUTTON_PIN     KEY_OK_Pin                /*!< GPIO pin user button       */
#define PLATFORM_USER_BUTTON_PORT    KEY_OK_GPIO_Port          /*!< GPIO port user button      */


/*
******************************************************************************
* GLOBAL MACROS
******************************************************************************
*/
#define platformProtectST25R391xComm()                do{ globalCommProtectCnt++; __DSB();NVIC_DisableIRQ(EXTI0_IRQn);__DSB();__ISB();}while(0) /*!< Protect unique access to ST25R391x communication channel - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment      */
#define platformUnprotectST25R391xComm()              do{ if (--globalCommProtectCnt==0U) {NVIC_EnableIRQ(EXTI0_IRQn);} }while(0)               /*!< Unprotect unique access to ST25R391x communication channel - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment */

#define platformProtectST25R391xIrqStatus()           platformProtectST25R391xComm()                /*!< Protect unique access to IRQ status var - IRQ disable on single thread environment (MCU) ; Mutex lock on a multi thread environment */
#define platformUnprotectST25R391xIrqStatus()         platformUnprotectST25R391xComm()              /*!< Unprotect the IRQ status var - IRQ enable on a single thread environment (MCU) ; Mutex unlock on a multi thread environment         */

#define platformProtectWorker()                                                                     /* Protect RFAL Worker/Task/Process from concurrent execution on multi thread platforms   */
#define platformUnprotectWorker()                                                                   /* Unprotect RFAL Worker/Task/Process from concurrent execution on multi thread platforms */


#define platformIrqST25R3911SetCallback( cb )          
#define platformIrqST25R3911PinInitialize()                

#define platformIrqST25R3916SetCallback( cb )          
#define platformIrqST25R3916PinInitialize()            


#define platformLedsInitialize()                                                                    /*!< Initializes the pins used as LEDs to outputs*/

#define platformLedOff( port, pin )                   platformGpioClear((port), (pin))              /*!< Turns the given LED Off                     */
#define platformLedOn( port, pin )                    platformGpioSet((port), (pin))                /*!< Turns the given LED On                      */
#define platformLedToogle( port, pin )                platformGpioToogle((port), (pin))             /*!< Toogle the given LED                        */

#define platformGpioSet( port, pin )                  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)    /*!< Turns the given GPIO High                   */
#define platformGpioClear( port, pin )                HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)  /*!< Turns the given GPIO Low                    */
#define platformGpioToogle( port, pin )               HAL_GPIO_TogglePin(port, pin)                 /*!< Toogles the given GPIO                      */
#define platformGpioIsHigh( port, pin )               (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) /*!< Checks if the given LED is High             */
#define platformGpioIsLow( port, pin )                (!platformGpioIsHigh(port, pin))              /*!< Checks if the given LED is Low              */

#define platformTimerCreate( t )                      timerCalculateTimer(t)                        /*!< Create a timer with the given time (ms)     */
#define platformTimerIsExpired( timer )               timerIsExpired(timer)                         /*!< Checks if the given timer is expired        */
#define platformDelay( t )                            HAL_Delay( t )                                /*!< Performs a delay for the given time (ms)    */

#define platformGetSysTick()                          HAL_GetTick()                                 /*!< Get System Tick ( 1 tick = 1 ms)            */

#define platformSpiSelect()                           platformGpioClear( ST25R391X_SS_PORT, ST25R391X_SS_PIN ) /*!< SPI SS\CS: Chip|Slave Select                */
#define platformSpiDeselect()                         platformGpioSet( ST25R391X_SS_PORT, ST25R391X_SS_PIN )   /*!< SPI SS\CS: Chip|Slave Deselect              */
#define platformSpiTxRx( txBuf, rxBuf, len )          spiTxRx( (txBuf), (rxBuf), (len) )            /*!< SPI transceive                              */


#define platformI2CTx( txBuf, len )                                                                 /*!< I2C Transmit                                */
#define platformI2CRx( txBuf, len )                                                                 /*!< I2C Receive                                 */
#define platformI2CStart()                                                                          /*!< I2C Start condition                         */
#define platformI2CStop()                                                                           /*!< I2C Stop condition                          */
#define platformI2CRepeatStart()                                                                    /*!< I2C Repeat Start                            */
#define platformI2CSlaveAddrWR(add)                                                                 /*!< I2C Slave address for Write operation       */
#define platformI2CSlaveAddrRD(add)                                                                 /*!< I2C Slave address for Read operation        */

//#define printf(...)                              logUsart(__VA_ARGS__)                         /*!< Log  method                                 */

/*
******************************************************************************
* GLOBAL VARIABLES
******************************************************************************
*/
extern uint8_t globalCommProtectCnt;                      /* Global Protection Counter provided per platform - instantiated in main.c    */

/*
******************************************************************************
* RFAL FEATURES CONFIGURATION
******************************************************************************
*/

#define RFAL_FEATURE_LISTEN_MODE               false      /*!< Enable/Disable RFAL support for Listen Mode                               */
#define RFAL_FEATURE_WAKEUP_MODE               true       /*!< Enable/Disable RFAL support for the Wake-Up mode                          */
#define RFAL_FEATURE_NFCA                      false       /*!< Enable/Disable RFAL support for NFC-A (ISO14443A)                         */
#define RFAL_FEATURE_NFCB                      false       /*!< Enable/Disable RFAL support for NFC-B (ISO14443B)                         */
#define RFAL_FEATURE_NFCF                      false       /*!< Enable/Disable RFAL support for NFC-F (FeliCa)                            */
#define RFAL_FEATURE_NFCV                      true       /*!< Enable/Disable RFAL support for NFC-V (ISO15693)                          */
#define RFAL_FEATURE_T1T                       false       /*!< Enable/Disable RFAL support for T1T (Topaz)                               */
#define RFAL_FEATURE_T2T                       false       /*!< Enable/Disable RFAL support for T2T                                       */
#define RFAL_FEATURE_T4T                       false       /*!< Enable/Disable RFAL support for T4T                                       */
#define RFAL_FEATURE_ST25TB                    false       /*!< Enable/Disable RFAL support for ST25TB                                    */
#define RFAL_FEATURE_ST25xV                    true       //CL/*!< Enable/Disable RFAL support for ST25TV/ST25DV                             */
#define RFAL_FEATURE_DYNAMIC_ANALOG_CONFIG     false      /*!< Enable/Disable Analog Configs to be dynamically updated (RAM)             */
#define RFAL_FEATURE_DYNAMIC_POWER             false      /*!< Enable/Disable RFAL dynamic power support                                 */
#define RFAL_FEATURE_ISO_DEP                   false       /*!< Enable/Disable RFAL support for ISO-DEP (ISO14443-4)                      */
#define RFAL_FEATURE_ISO_DEP_POLL              false       /*!< Enable/Disable RFAL support for Poller mode (PCD) ISO-DEP (ISO14443-4)    */
#define RFAL_FEATURE_ISO_DEP_LISTEN            false      /*!< Enable/Disable RFAL support for Listen mode (PICC) ISO-DEP (ISO14443-4)   */
#define RFAL_FEATURE_NFC_DEP                   false       /*!< Enable/Disable RFAL support for NFC-DEP (NFCIP1/P2P)                      */


#define RFAL_FEATURE_ISO_DEP_IBLOCK_MAX_LEN    256U       /*!< ISO-DEP I-Block max length. Please use values as defined by rfalIsoDepFSx */
#define RFAL_FEATURE_ISO_DEP_APDU_MAX_LEN      1024U      /*!< ISO-DEP APDU max length. Please use multiples of I-Block max length       */

#endif /* PLATFORM_H */


