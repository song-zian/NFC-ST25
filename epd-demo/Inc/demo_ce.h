/**
  ******************************************************************************
  * @file    demo_ce.h 
  * @author  MMY Application Team
  * @brief   Implementation of Common CardEmulation parts
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DEMO_CE_H
#define DEMO_CE_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/
#include "platform.h"

/** @addtogroup X-CUBE-NFC6_Applications
 *  @brief Sample applications for X-NUCLEO-NFC06A1 STM32 expansion boards.
 *  @{
 */

/** @addtogroup CardEmulation
 *  @{
 */

/** @defgroup CE_CardEmul
 *  @brief Card Emulation management functions
 * @{
 */


/* Exported constants --------------------------------------------------------*/
/* T3T Information Block Bytes offset */
#define T3T_INFBLK_VER_OFFSET       0
#define T3T_INFBLK_NBR_OFFSET       1
#define T3T_INFBLK_NBW_OFFSET       2
#define T3T_INFBLK_NMAXB_OFFSET     3
#define T3T_INFBLK_WRITEFLAG_OFFSET 9
#define T3T_INFBLK_RWFLAG_OFFSET    10
#define T3T_INFBLK_LN_OFFSET        11
#define T3T_INFBCK_CHECKSUM_OFFSET  14

/* T3T Information Block WriteFlag values */
#define T3T_WRITEFLAG_OFF 0x00
#define T3T_WRITEFLAG_ON  0x0F

/* T3T COMMAND OFFSET */
#define T3T_CHECK_RESP_CMD_OFFSET    0
#define T3T_CHECK_RESP_NFCID2_OFFSET 1
#define T3T_CHECK_RESP_SF1_OFFSET    9
#define T3T_CHECK_RESP_SF2_OFFSET    10
#define T3T_CHECK_RESP_NOB_OFFSET    11
#define T3T_CHECK_RESP_DATA_OFFSET   12
#define T3T_UPDATE_RESP_CMD_OFFSET    0
#define T3T_UPDATE_RESP_NFCID2_OFFSET 1
#define T3T_UPDATE_RESP_SF1_OFFSET    9
#define T3T_UPDATE_RESP_SF2_OFFSET    10

/* External variables --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/** @defgroup CE_CardEmul_Exported_functions
 *  @{
 */
void     demoCeInit(uint8_t* nfcfNfcid);
uint16_t demoCeT3T(uint8_t *rxData, uint16_t rxDataLen, uint8_t *txBuf, uint16_t txBufLen );
uint16_t demoCeT4T(uint8_t *rxData, uint16_t rxDataLen, uint8_t *txBuf, uint16_t txBufLen );



/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
  }
#endif

#endif /* DEMO_CE_H */

/******************* (C) COPYRIGHT 2018 STMicroelectronics *****END OF FILE****/
