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
 *  \brief NDEF RTD (well-known and external) types header file
 *
 * NDEF RTD provides functionalities to handle RTD records, such as Text or URI.
 *
 * \addtogroup NDEF
 * @{
 *
 */

#ifndef NDEF_TYPES_RTD_H
#define NDEF_TYPES_RTD_H


/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "ndef_types.h"
#include "ndef_record.h"


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

/*! RTD Record Type buffers */
extern const ndefConstBuffer8 bufRtdTypeDeviceInfo; /*! Device Information Record Type buffer               */
extern const ndefConstBuffer8 bufRtdTypeText;       /*! Text Record Type buffer                             */
extern const ndefConstBuffer8 bufRtdTypeUri;        /*! URI Record Type buffer                              */
extern const ndefConstBuffer8 bufRtdTypeAar;        /*! AAR (Android Application Record) Record Type buffer */


/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/***************
 * Empty type
 ***************
 */

/*!
 *****************************************************************************
 * Initialize an Empty type
 *
 * \param[out] empty: Type to initialize
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefEmptyType(ndefType* empty);


/*!
 *****************************************************************************
 * Convert an NDEF record to an Empty type
 *
 * \param[in]  record: Record to convert
 * \param[out] empty:  The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToEmptyType(const ndefRecord* record, ndefType* empty);


/*!
 *****************************************************************************
 * Convert an Empty type to an NDEF record
 *
 * \param[in]  empty:  Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefEmptyTypeToRecord(const ndefType* empty, ndefRecord* record);


/*********************
 * Device Information
 *********************
 */

/*!
 *****************************************************************************
 * Initialize a RTD Device Information type
 *
 * \param[out] devInfo:          Type to initialize
 * \param[in]  devInfoData:      Device Information data
 * \param[in]  devInfoDataCount: Number of Device Information data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfo(ndefType* devInfo, const ndefDeviceInfoEntry* devInfoData, uint8_t devInfoDataCount);


/*!
 *****************************************************************************
 * Get RTD Device Information type content
 *
 * \param[in]  devInfo:     Type to get information from
 * \param[out] devInfoData: Device Information data
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdDeviceInfo(const ndefType* devInfo, ndefTypeRtdDeviceInfo* devInfoData);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Device Information RTD type
 *
 * \param[in]  record:  Record to convert
 * \param[out] devInfo: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdDeviceInfo(const ndefRecord* record, ndefType* devInfo);


/*!
 *****************************************************************************
 * Convert a Device Information RTD type to an NDEF record
 *
 * \param[in]  devInfo: Type to convert
 * \param[out] record:  The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdDeviceInfoToRecord(const ndefType* devInfo, ndefRecord* record);


/***************
 * Text
 ***************
 */


/*!
 *****************************************************************************
 * Initialize a Text RTD type
 *
 * \param[out] text:            Type to initialize
 * \param[out] utfEncoding:     UTF-8/UTF-16
 * \param[in]  bufLanguageCode: ISO/IANA language code buffer
 * \param[in]  bufSentence:     Actual text buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdText(ndefType* text, uint8_t utfEncoding, const ndefConstBuffer8* bufLanguageCode, const ndefConstBuffer* bufSentence);


/*!
 *****************************************************************************
 * Get RTD Text type content
 *
 * \param[in]  text:            Type to get information from
 * \param[out] utfEncoding:     UTF-8/UTF-16
 * \param[out] bufLanguageCode: ISO/IANA language code buffer
 * \param[out] bufSentence:     Actual text buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdText(const ndefType* text, uint8_t* utfEncoding, ndefConstBuffer8* bufLanguageCode, ndefConstBuffer* bufSentence);


/*!
 *****************************************************************************
 * Convert an NDEF record to a Text type
 *
 * \param[in]  record: Record to convert
 * \param[out] text:   The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdText(const ndefRecord* record, ndefType* text);


/*!
 *****************************************************************************
 * Convert a Text RTD type to an NDEF record
 *
 * \param[in]  text:   Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdTextToRecord(const ndefType* text, ndefRecord* record);


/***************
 * URI
 ***************
 */

/*!
 *****************************************************************************
 * Initialize a URI RTD type
 *
 * \param[out] uri:          Type to initialize
 * \param[in]  protocol:     URI protocol
 * \param[in]  bufUriString: URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdUri(ndefType* uri, uint8_t protocol, const ndefConstBuffer* bufUriString);


/*!
 *****************************************************************************
 * Get URI RTD type content
 *
 * \param[in]  uri:          Type to get information from
 * \param[out] bufProtocol:  URI protocol buffer
 * \param[out] bufUriString: URI string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdUri(const ndefType* uri, ndefConstBuffer* bufProtocol, ndefConstBuffer* bufUriString);


/*!
 *****************************************************************************
 * Convert an NDEF record to a URI RTD type
 *
 * \param[in]  record: Record to convert
 * \param[out] uri:    The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdUri(const ndefRecord* record, ndefType* uri);


/*!
 *****************************************************************************
 * Convert a URI RTD type to an NDEF record
 *
 * \param[in]  uri:    Type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdUriToRecord(const ndefType* uri, ndefRecord* record);


/*******************
 * AAR External Type
 *******************
 */

/*!
 *****************************************************************************
 * Initialize an RTD Android Application Record External type
 *
 * \param[out] aar:        Type to initialize
 * \param[in]  bufPayload: Payload buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdAar(ndefType* aar, const ndefConstBuffer* bufPayload);


/*!
 *****************************************************************************
 * Get RTD Android Application Record type content
 *
 * \param[in]  aar:          Type to get information from
 * \param[out] bufAarString: AAR string buffer
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefGetRtdAar(const ndefType* aar, ndefConstBuffer* bufAarString);


/*!
 *****************************************************************************
 * Convert an NDEF record to an RTD Android Application Record External type
 *
 * \param[in]  record: Record to convert
 * \param[out] aar:    The converted AAR external type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRecordToRtdAar(const ndefRecord* record, ndefType* aar);


/*!
 *****************************************************************************
 * Convert an RTD Android Application Record External type to an NDEF record
 *
 * \param[in]  aar:    AAR External type to convert
 * \param[out] record: The converted type
 *
 * \return ERR_NONE if successful or a standard error code
 *****************************************************************************
 */
ReturnCode ndefRtdAarToRecord(const ndefType* aar, ndefRecord* record);


#endif /* NDEF_TYPES_RTD_H */

/**
  * @}
  *
  */
