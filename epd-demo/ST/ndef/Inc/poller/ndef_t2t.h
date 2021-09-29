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

/*
 *      PROJECT:   NDEF firmware
 *      Revision:
 *      LANGUAGE:  ISO C99
 */

/*! \file 
 *
 *  \author 
 *
 *  \brief Provides NDEF methods and definitions to access NFC Forum T2T
 *  
 *  NDEF T2T provides several functionalities required to 
 *  perform NDEF message management with T2T tags.
 *  
 *  The most common interfaces are
 *    <br>&nbsp; ndefT2TPollerContextInitialization()
 *    <br>&nbsp; ndefT2TPollerNdefDetect()
 *    <br>&nbsp; ndefT2TPollerReadRawMessage()
 *    <br>&nbsp; ndefT2TPollerWriteRawMessage()
 *    <br>&nbsp; ndefT2TPollerTagFormat()
 *  
 *  
 * \addtogroup NDEF
 * @{
 *  
 */


#ifndef NDEF_T2T_H
#define NDEF_T2T_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "platform.h"
#include "st_errno.h"
#include "rfal_nfca.h"
#include "rfal_t2t.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

 /*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

/*!
 *****************************************************************************
 * \brief Handle T2T NDEF context activation
 *  
 * This method performs the initialization of the NDEF context and handles 
 * the activation of the ISO-DEP layer. It must be called after a successful 
 * anti-collision procedure and prior to any NDEF procedures such as NDEF 
 * detection procedure.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   dev    : ndef Device
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev);


/*!
 *****************************************************************************
 * \brief T2T NDEF Detection procedure
 *  
 * This method performs the T2T NDEF Detection procedure
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : Detection failed (application or ccfile not found)
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief T2T Read data from tag memory
 *  
 * This method reads arbitrary length data from the current selected file 
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   len    : requested len 
 * \param[in]   offset : file offset of where to start reading data
 * \param[out]  buf    : buffer to place the data read from the tag
 * \param[out]  rcvdLen: received length
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief T2T write data to tag memory
 *  
 * This method reads arbitrary length data from the current selected file 
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to start writing data
 * \param[in]   buf    : data to write
 * \param[in]   len    : buf len 
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len);


/*!
 *****************************************************************************
 * \brief T2T Read raw NDEF message
 *  
 * This method reads a raw NDEF message from the current selected file.
 * Prior to NDEF Read procedure, a successful ndefT2TPollerNdefDetect() 
 * has to be performed.
 * 
 * \param[in]   ctx    : ndef Context
 * \param[out]  buf    : buffer to place the NDEF message
 * \param[in]   bufLen : buffer length
 * \param[out]  rcvdLen: received length
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief T2T Write raw NDEF message
 *  
 * This method writes a raw NDEF message in the current selected file.
 * Prior to NDEF Write procedure, a successful ndefT2TPollerNdefDetect() 
 * has to be performed.
 * 
 * \warning Current selected file must not be changed between NDEF Detect 
 * procedure and NDEF Write procedure. If another file is selected before 
 * NDEF Write procedure, it is user responsibility to re-select NDEF file
 * or to call ndefT2TPollerNdefDetect() to restore proper context.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   buf    : raw message buffer
 * \param[in]   bufLen : buffer length
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief T2T Write NDEF message length
 *  
 * This method writes the NLEN field (V2 mapping) or the ENLEN (V3 mapping).
 *
 * \param[in]   ctx          : ndef Context
 * \param[in]   rawMessageLen: len
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen);
 
 
/*!
 *****************************************************************************
 * \brief T2T Format Tag
 *  
 * This method formats a tag to make it ready for NDEF storage. 
 * The Capability Container block is written only for virgin tags.
 * If the cc parameter is not provided (i.e. NULL), a default one is used 
 * with T2T_AreaSize = 48 bytes.
 * Beware that formatting is on most tags a one time operation (OTP bits!!!!)
 * Doing a wrong format may render your tag unusable.
 * options parameter is not used for T2T Tag Format method
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   cc     : Capability Container
 * \param[in]   options: specific flags
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief T2T Check Presence
 *  
 * This method checks whether a T2T tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context
 * 
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefT2TPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief T2T Check Available Space
 *  
 * This method checks whether a T2T tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 * 
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT2TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T2T Begin Write Message
 *  
 * This method sets the L-field to 0 and sets the message offset to the proper value according to messageLen
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 * 
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT2TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief T2T End Write Message
 *  
 * This method updates the L-field value after the message has been written
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 * 
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefT2TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen);


#endif /* NDEF_T2T_H */

/**
  * @}
  */
