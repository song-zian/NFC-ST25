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
 *  \brief NDEF RTD and MIME types
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "platform.h"
#include "st_errno.h"
#include "utils.h"
#include "ndef_record.h"
#include "ndef_types.h"
#include "ndef_types_rtd.h"
#include "ndef_types_mime.h"
#include "ndef_type_wifi.h"


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


/*! NDEF type table to associate a ndefTypeId and a string */
typedef struct
{
    ndefTypeId              typeId;        /*!< NDEF Type Id       */
    uint8_t                 tnf;           /*!< TNF                */
    const ndefConstBuffer8* bufTypeString; /*!< Type String buffer */
} ndefTypeTable;


/*! Type wrapper to function pointers */
typedef struct
{
    ndefTypeId typeId;                                                    /*!< NDEF Type Id              */
    ReturnCode (*recordToType)(const ndefRecord* record, ndefType* type); /*!< Pointer to read function  */
    ReturnCode (*typeToRecord)(const ndefType* type, ndefRecord* record); /*!< Pointer to write function */
} ndefTypeConverter;


/*! Array to match RTD strings with Well-known types, and converting functions */
static const ndefTypeConverter typeConverterTable[] =
{
    { NDEF_TYPE_EMPTY,           ndefRecordToEmptyType,       ndefEmptyTypeToRecord       },
    { NDEF_TYPE_RTD_DEVICE_INFO, ndefRecordToRtdDeviceInfo,   ndefRtdDeviceInfoToRecord   },
    { NDEF_TYPE_RTD_TEXT,        ndefRecordToRtdText,         ndefRtdTextToRecord         },
    { NDEF_TYPE_RTD_URI,         ndefRecordToRtdUri,          ndefRtdUriToRecord          },
    { NDEF_TYPE_RTD_AAR,         ndefRecordToRtdAar,          ndefRtdAarToRecord          },
    { NDEF_TYPE_MEDIA_VCARD,     ndefRecordToVCard,           ndefVCardToRecord           },
    { NDEF_TYPE_MEDIA_WIFI,      ndefRecordToWifi,            ndefWifiToRecord            },
};


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */


/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */


/*****************************************************************************/
static ReturnCode ndefTypeStringToTypeId(uint8_t tnf, const ndefConstBuffer8* bufTypeString, ndefTypeId* typeId)
{
    /*! Empty string */
    static const uint8_t    ndefTypeEmpty[] = "";    /*!< Empty string */
    static ndefConstBuffer8 bufTypeEmpty    = { ndefTypeEmpty, sizeof(ndefTypeEmpty) - 1U };

    // TODO Transform the enum (u32) to defines (u8), re-order to u32-u8-u8 to compact buffer !
    static const ndefTypeTable typeTable[] =
    {
        { NDEF_TYPE_EMPTY,           NDEF_TNF_EMPTY,               &bufTypeEmpty              },
        { NDEF_TYPE_RTD_DEVICE_INFO, NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeDeviceInfo      },
        { NDEF_TYPE_RTD_TEXT,        NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeText            },
        { NDEF_TYPE_RTD_URI,         NDEF_TNF_RTD_WELL_KNOWN_TYPE, &bufRtdTypeUri             },
        { NDEF_TYPE_RTD_AAR,         NDEF_TNF_RTD_EXTERNAL_TYPE,   &bufRtdTypeAar             },
        { NDEF_TYPE_MEDIA_VCARD,     NDEF_TNF_MEDIA_TYPE,          &bufMediaTypeVCard         },
        { NDEF_TYPE_MEDIA_WIFI,      NDEF_TNF_MEDIA_TYPE,          &bufMediaTypeWifi          },
    };

    uint32_t i;

    if ( (bufTypeString == NULL) || (typeId == NULL) )
    {
        return ERR_PROTO;
    }

    for (i = 0; i < SIZEOF_ARRAY(typeTable); i++)
    {
        /* Check TNF and length are the same, then compare the content */
        if (typeTable[i].tnf == tnf)
        {
            if (bufTypeString->length == typeTable[i].bufTypeString->length)
            {
                if (bufTypeString->length == 0U)
                {
                    /* Empty type */
                    *typeId = typeTable[i].typeId;
                    return ERR_NONE;
                }
                else
                {
                    if (ST_BYTECMP(typeTable[i].bufTypeString->buffer, bufTypeString->buffer, bufTypeString->length) == 0)
                    {
                        *typeId = typeTable[i].typeId;
                        return ERR_NONE;
                    }
                }
            }
        }
    }

    return ERR_NOTFOUND;
}


/*****************************************************************************/
ReturnCode ndefRecordTypeStringToTypeId(const ndefRecord* record, ndefTypeId* typeId)
{
    ReturnCode err;

    uint8_t          tnf;
    ndefConstBuffer8 bufRecordType;

    if ( (record == NULL) || (typeId == NULL) )
    {
        return ERR_PARAM;
    }

    err = ndefRecordGetType(record, &tnf, &bufRecordType);
    if (err != ERR_NONE)
    {
        return err;
    }
    if (tnf >= NDEF_TNF_RESERVED)
    {
        return ERR_INTERNAL;
    }

    switch (tnf)
    {
    case NDEF_TNF_EMPTY:               /* Fall through */
    case NDEF_TNF_RTD_WELL_KNOWN_TYPE: /* Fall through */
    case NDEF_TNF_RTD_EXTERNAL_TYPE:   /* Fall through */
    case NDEF_TNF_MEDIA_TYPE:          /* Fall through */
        err = ndefTypeStringToTypeId(tnf, &bufRecordType, typeId);
        break;
    default:
        err = ERR_NOT_IMPLEMENTED;
        break;
    }

    return err;
}


/*****************************************************************************/
ReturnCode ndefRecordToType(const ndefRecord* record, ndefType* type)
{
    const ndefType* ndeftype;
    ReturnCode err;
    ndefTypeId typeId;
    uint32_t   i;

    ndeftype = ndefRecordGetNdefType(record);
    if (ndeftype != NULL)
    {
        /* Return the well-known type contained in the record */
        (void)ST_MEMCPY(type, ndeftype, sizeof(ndefType));
        return ERR_NONE;
    }

    err = ndefRecordTypeStringToTypeId(record, &typeId);
    if (err != ERR_NONE)
    {
        return err;
    }

    for (i = 0; i < SIZEOF_ARRAY(typeConverterTable); i++)
    {
        if (typeId == typeConverterTable[i].typeId)
        {
            /* Call the appropriate function to the matching type */
            if (typeConverterTable[i].recordToType != NULL)
            {
                return typeConverterTable[i].recordToType(record, type);
            }
        }
    }

    return ERR_NOT_IMPLEMENTED;
}


/*****************************************************************************/
ReturnCode ndefTypeToRecord(const ndefType* type, ndefRecord* record)
{
    uint32_t i;

    if (type == NULL)
    {
        return ERR_PARAM;
    }

    for (i = 0; i < SIZEOF_ARRAY(typeConverterTable); i++)
    {
        if (type->id == typeConverterTable[i].typeId)
        {
            /* Call the appropriate function to the matching type */
            if (typeConverterTable[i].typeToRecord != NULL)
            {
                return typeConverterTable[i].typeToRecord(type, record);
            }
        }
    }

    return ERR_NOT_IMPLEMENTED;
}


/*****************************************************************************/
ReturnCode ndefRecordSetNdefType(ndefRecord* record, const ndefType* type)
{
    uint32_t payloadLength;

    if ( (record == NULL) ||
         (type                   == NULL)               ||
         (type->id                > NDEF_TYPE_ID_COUNT) ||
         (type->getPayloadLength == NULL)               ||
         (type->getPayloadItem   == NULL) )
    {
        return ERR_PARAM;
    }

    record->ndeftype = type;

    /* Set Short Record bit accordingly */
    payloadLength = ndefRecordGetPayloadLength(record);
    ndefHeaderSetValueSR(record, (payloadLength <= NDEF_SHORT_RECORD_LENGTH_MAX) ? 1 : 0);

    return ERR_NONE;
}


/*****************************************************************************/
const ndefType* ndefRecordGetNdefType(const ndefRecord* record)
{
    if (record == NULL)
    {
        return NULL;
    }

    if (record->ndeftype != NULL)
    {
        /* Check whether it is a valid NDEF type */
        if ( (record->ndeftype->id                < NDEF_TYPE_ID_COUNT) &&
             (record->ndeftype->getPayloadItem   != NULL) &&
             (record->ndeftype->getPayloadLength != NULL) )
        {
            return record->ndeftype;
        }
    }

    return NULL;
}
