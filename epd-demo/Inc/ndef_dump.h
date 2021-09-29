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
 *  \brief NDEF message dump utils header file
 *
 */

#ifndef NDEF_DUMP_H
#define NDEF_DUMP_H


/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "platform.h"
#include "ndef_record.h"
#include "ndef_message.h"
#include "ndef_types.h"

/*
 ******************************************************************************
 * GLOBAL DEFINES
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
 * Dump an NDEF record
 *
 * This function dumps an NDEF record in a formatted readable style
 * The verbose selector enables more or less dump output.
 *
 * \param[in] record
 * \param[in] verbose
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordDump(const ndefRecord* record, bool verbose);

/*!
 *****************************************************************************
 * Dump an NDEF message
 *
 * This function dumps an NDEF message in a formatted readable style
 * The verbose selector enables more or less dump output.
 *
 * \param[in] message
 * \param[in] verbose
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMessageDump(const ndefMessage* message, bool verbose);

/*!
 *****************************************************************************
 * Dump an Empty type
 *
 * \param[in] empty: Type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefEmptyTypeDump(const ndefType* empty);

/*!
 *****************************************************************************
 * Dump a Device Information RTD well-known type
 *
 * \param[in] devInfo: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfoDump(const ndefType* devInfo);

/*!
 *****************************************************************************
 * Dump a Text RTD well-known type
 *
 * \param[in] text: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTextDump(const ndefType* text);

/*!
 *****************************************************************************
 * Dump a URI RTD well-known type
 *
 * \param[in] uri: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdUriDump(const ndefType* uri);

/*!
 *****************************************************************************
 * Dump an External RTD type
 *
 * \param[in] ext: Well-known type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdAarDump(const ndefType* ext);

/*!
 *****************************************************************************
 * Dump a Media type
 *
 * \param[in] media: Media type to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMediaTypeDump(const ndefType* media);

/*!
 *****************************************************************************
 * Dump a vCard Media type
 *
 * \param[in] vCard: vCard to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMediaVCardDump(const ndefType* vCard);

/*!
 *****************************************************************************
 * Dump a Wifi Media type
 *
 * \param[in] wifi: Wifi parameters to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefMediaWifiDump(const ndefType* wifi);

/*!
 *****************************************************************************
 * Dump a well-known type
 *
 * \param[in] record: Record to dump
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordDumpType(const ndefRecord* record);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] string:     Message displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] verbose     Increase the lenght displayed
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBufferDump(const char* string, const ndefConstBuffer* bufPayload, bool verbose);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] prefix:     String displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] suffix:     String displayed after the buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBufferPrint(const char* prefix, const ndefConstBuffer* bufPayload, const char* suffix);

/*!
 *****************************************************************************
 * Dump a raw buffer stored in a ndefBuffer8
 *
 * This function dumps a raw buffer in a formatted style
 *
 * \param[in] prefix:     String displayed before the buffer
 * \param[in] bufPayload: Payload buffer to display
 * \param[in] suffix:     String displayed after the buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefBuffer8Print(const char* prefix, const ndefConstBuffer8* bufPayload, const char* suffix);


#endif /* NDEF_DUMP_H */
