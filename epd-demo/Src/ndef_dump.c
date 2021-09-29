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
 *  \brief NDEF message dump utils
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
#include "ndef_message.h"
#include "ndef_types_rtd.h"
#include "ndef_types_mime.h"
#include "ndef_dump.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

/*! Table to associate enums to pointer to function */
typedef struct
{
    ndefTypeId typeId;                        /*!< NDEF type Id             */
    ReturnCode (*dump)(const ndefType* type); /*!< Pointer to dump function */
} ndefTypeDumpTable;


static const ndefTypeDumpTable typeDumpTable[] =
{
    { NDEF_TYPE_EMPTY,           ndefEmptyTypeDump       },
    { NDEF_TYPE_RTD_DEVICE_INFO, ndefRtdDeviceInfoDump   },
    { NDEF_TYPE_RTD_TEXT,        ndefRtdTextDump         },
    { NDEF_TYPE_RTD_URI,         ndefRtdUriDump          },
    { NDEF_TYPE_RTD_AAR,         ndefRtdAarDump          },
    { NDEF_TYPE_MEDIA_VCARD,     ndefMediaVCardDump      },
    { NDEF_TYPE_MEDIA_WIFI,      ndefMediaWifiDump       },
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
static bool isPrintableASCII(const uint8_t* str, uint32_t strLen)
{
    uint32_t i;
    
    if ((str == NULL) || (strLen == 0))
    {
        return false;
    }

    for (i = 0; i < strLen; i++)
    {
        if ((str[i] < 0x20) || (str[i] > 0x7E))
        {
            return false;
        }
    }

    return true;
}


/*****************************************************************************/
ReturnCode ndefRecordDump(const ndefRecord* record, bool verbose)
{
    static uint32_t index;
    const uint8_t *ndefTNFNames[] =
    {
        (uint8_t *)"Empty",
        (uint8_t *)"NFC Forum well-known type [NFC RTD]",
        (uint8_t *)"Media-type as defined in RFC 2046",
        (uint8_t *)"Absolute URI as defined in RFC 3986",
        (uint8_t *)"NFC Forum external type [NFC RTD]",
        (uint8_t *)"Unknown",
        (uint8_t *)"Unchanged",
        (uint8_t *)"Reserved"
    };
    uint8_t* headerSR = (uint8_t*)"";
    ReturnCode err;

    if (record == NULL)
    {
        platformLog("No record\r\n");
        return ERR_NONE;
    }

    if (ndefHeaderIsSetMB(record))
    {
        index = 1U;
    }
    else
    {
        index++;
    }

    if (verbose == true)
    {
        headerSR = (uint8_t*)(ndefHeaderIsSetSR(record) ? " - Short Record" : " - Standard Record");
    }

    platformLog("Record #%d%s\r\n", index, headerSR);

    /* Well-known type dump */
    err = ndefRecordDumpType(record);
    if (verbose == true)
    {
        /* Raw dump */
        //platformLog(" MB:%d ME:%d CF:%d SR:%d IL:%d TNF:%d\r\n", ndefHeaderMB(record), ndefHeaderME(record), ndefHeaderCF(record), ndefHeaderSR(record), ndefHeaderIL(record), ndefHeaderTNF(record));
        platformLog(" MB ME CF SR IL TNF\r\n");
        platformLog("  %d  %d  %d  %d  %d   %d\r\n", ndefHeaderMB(record), ndefHeaderME(record), ndefHeaderCF(record), ndefHeaderSR(record), ndefHeaderIL(record), ndefHeaderTNF(record));
    }
    if ( (err != ERR_NONE) || (verbose == true) )
    {
        platformLog(" Type Name Format: %s\r\n", ndefTNFNames[ndefHeaderTNF(record)]);

        uint8_t tnf;
        ndefConstBuffer8 bufRecordType;
        ndefRecordGetType(record, &tnf, &bufRecordType);
        if ( (tnf == NDEF_TNF_EMPTY) && (bufRecordType.length == 0U) )
        {
            platformLog(" Empty NDEF record\r\n");
        }
        else
        {
            ndefBuffer8Print(" Type: \"", &bufRecordType, "\"\r\n");
        }

        if (ndefHeaderIsSetIL(record))
        {
            /* ID Length bit set */
            ndefConstBuffer8 bufRecordId;
            ndefRecordGetId(record, &bufRecordId);
            ndefBuffer8Print(" ID: \"", &bufRecordId, "\"\r\n");
        }

        ndefConstBuffer bufRecordPayload;
        ndefRecordGetPayload(record, &bufRecordPayload);
        ndefBufferDump(" Payload:", &bufRecordPayload, verbose);
    }

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMessageDump(const ndefMessage* message, bool verbose)
{
    ReturnCode  err;
    ndefRecord* record;

    if (message == NULL)
    {
        platformLog("Empty NDEF message\r\n");
        return ERR_NONE;
    }
    else
    {
        platformLog("Decoding NDEF message\r\n");
    }

    record = ndefMessageGetFirstRecord(message);

    while (record != NULL)
    {
        err = ndefRecordDump(record, verbose);
        if (err != ERR_NONE)
        {
            return err;
        }
        record = ndefMessageGetNextRecord(record);
    }

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefEmptyTypeDump(const ndefType* empty)
{
    if (empty == NULL)
    {
        return ERR_PARAM;
    }

    if (empty->id != NDEF_TYPE_EMPTY)
    {
        return ERR_PARAM;
    }

    platformLog(" Empty record\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRtdDeviceInfoDump(const ndefType* devInfo)
{
    ndefTypeRtdDeviceInfo devInfoData;
    uint32_t type;
    uint32_t i;

    const uint8_t* ndefDeviceInfoName[] =
    {
        (uint8_t*)"Manufacturer",
        (uint8_t*)"Model",
        (uint8_t*)"Device",
        (uint8_t*)"UUID",
        (uint8_t*)"Firmware version",
    };

    if (devInfo == NULL)
    {
        return ERR_PARAM;
    }

    if (devInfo->id != NDEF_TYPE_RTD_DEVICE_INFO)
    {
        return ERR_PARAM;
    }

    ndefGetRtdDeviceInfo(devInfo, &devInfoData);

    platformLog(" Device Information:\r\n");

    for (type = 0; type < NDEF_DEVICE_INFO_TYPE_COUNT; type++)
    {
        if (devInfoData.devInfo[type].buffer != NULL)
        {
            platformLog(" - %s: ", ndefDeviceInfoName[devInfoData.devInfo[type].type]);

            if (type != NDEF_DEVICE_INFO_UUID)
            {
                for (i = 0; i < devInfoData.devInfo[type].length; i++)
                {
                    platformLog("%c", devInfoData.devInfo[type].buffer[i]); /* character */
                }
            }
            else
            {
                for (i = 0; i < devInfoData.devInfo[type].length; i++)
                {
                    platformLog("%.2X", devInfoData.devInfo[type].buffer[i]); /* hex number */
                }
            }
            platformLog("\r\n");
        }
    }

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRtdTextDump(const ndefType* text)
{
    uint8_t utfEncoding;
    ndefConstBuffer8 bufLanguageCode;
    ndefConstBuffer  bufSentence;

    if (text == NULL)
    {
        return ERR_PARAM;
    }

    if (text->id != NDEF_TYPE_RTD_TEXT)
    {
        return ERR_PARAM;
    }

    ndefGetRtdText(text, &utfEncoding, &bufLanguageCode, &bufSentence);

    ndefBufferPrint(" Text: \"", &bufSentence, "");

    platformLog("\" (%s,", utfEncoding == TEXT_ENCODING_UTF8 ? "UTF8" : "UTF16");

    ndefBuffer8Print(" language code \"", &bufLanguageCode, "\")\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRtdUriDump(const ndefType* uri)
{
    ndefConstBuffer bufProtocol;
    ndefConstBuffer bufUriString;

    if (uri == NULL)
    {
        return ERR_PARAM;
    }

    if (uri->id != NDEF_TYPE_RTD_URI)
    {
        return ERR_PARAM;
    }

    ndefGetRtdUri(uri, &bufProtocol, &bufUriString);

    ndefBufferPrint("URI: (", &bufProtocol, ")");
    ndefBufferPrint("", &bufUriString, "\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRtdAarDump(const ndefType* aar)
{
    ndefConstBuffer bufAarString;

    if (aar == NULL)
    {
        return ERR_PARAM;
    }

    if (aar->id != NDEF_TYPE_RTD_AAR)
    {
        return ERR_PARAM;
    }

    ndefGetRtdAar(aar, &bufAarString);

    ndefBufferPrint(" AAR Package: ", &bufAarString, "\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMediaTypeDump(const ndefType* media)
{
    ndefConstBuffer8 bufType;
    ndefConstBuffer  bufPayload;

    if (media == NULL)
    {
        return ERR_PARAM;
    }

    if (media->id != NDEF_TYPE_MEDIA)
    {
        return ERR_PARAM;
    }

    ndefGetMedia(media, &bufType, &bufPayload);

    ndefBuffer8Print(" Media Type: ", &bufType, "\r\n");
    ndefBufferPrint(" Payload: ", &bufPayload, "\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
static ReturnCode ndefMediaVCardTranslate(const ndefConstBuffer* bufText, ndefConstBuffer* bufTranslation)
{
    typedef struct {
        uint8_t* vCardString;
        uint8_t* english;
    } ndefTranslate;

    const ndefTranslate translate[] =
    {
        { (uint8_t*)"N"            , (uint8_t*)"Name"           },
        { (uint8_t*)"FN"           , (uint8_t*)"Formatted Name" },
        { (uint8_t*)"ADR"          , (uint8_t*)"Address"        },
        { (uint8_t*)"TEL"          , (uint8_t*)"Phone"          },
        { (uint8_t*)"EMAIL"        , (uint8_t*)"Email"          },
        { (uint8_t*)"TITLE"        , (uint8_t*)"Title"          },
        { (uint8_t*)"ORG"          , (uint8_t*)"Org"            },
        { (uint8_t*)"URL"          , (uint8_t*)"URL"            },
        { (uint8_t*)"PHOTO"        , (uint8_t*)"Photo"          },
    };

    uint32_t i;

    if ( (bufText == NULL) || (bufTranslation == NULL) )
    {
        return ERR_PROTO;
    }

    for (i = 0; i < SIZEOF_ARRAY(translate); i++)
    {
        if (ST_BYTECMP(bufText->buffer, translate[i].vCardString, strlen((char*)translate[i].vCardString)) == 0)
        {
            bufTranslation->buffer = translate[i].english;
            bufTranslation->length = strlen((char*)translate[i].english);

            return ERR_NONE;
        }
    }

    bufTranslation->buffer = bufText->buffer;
    bufTranslation->length = bufText->length;

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMediaVCardDump(const ndefType* vCard)
{
    ndefConstBuffer bufTypeN     = { (uint8_t*)"N",     strlen((char*)"N")     };
    ndefConstBuffer bufTypeFN    = { (uint8_t*)"FN",    strlen((char*)"FN")    };
    ndefConstBuffer bufTypeADR   = { (uint8_t*)"ADR",   strlen((char*)"ADR")   };
    ndefConstBuffer bufTypeTEL   = { (uint8_t*)"TEL",   strlen((char*)"TEL")   };
    ndefConstBuffer bufTypeEMAIL = { (uint8_t*)"EMAIL", strlen((char*)"EMAIL") };
    ndefConstBuffer bufTypeTITLE = { (uint8_t*)"TITLE", strlen((char*)"TITLE") };
    ndefConstBuffer bufTypeORG   = { (uint8_t*)"ORG",   strlen((char*)"ORG")   };
    ndefConstBuffer bufTypeURL   = { (uint8_t*)"URL",   strlen((char*)"URL")   };
    ndefConstBuffer bufTypePHOTO = { (uint8_t*)"PHOTO", strlen((char*)"PHOTO") };

    const ndefConstBuffer* bufVCardField[] = {
        &bufTypeN    ,
        &bufTypeFN   ,
        &bufTypeADR  ,
        &bufTypeTEL  ,
        &bufTypeEMAIL,
        &bufTypeTITLE,
        &bufTypeORG  ,
        &bufTypeURL  ,
        &bufTypePHOTO,
    };

    uint32_t i;
    const ndefConstBuffer* bufType;
    ndefConstBuffer        bufSubType;
    ndefConstBuffer        bufValue;

    if (vCard == NULL)
    {
        return ERR_PARAM;
    }

    if (vCard->id != NDEF_TYPE_MEDIA_VCARD)
    {
        return ERR_PARAM;
    }

    platformLog(" vCard decoded: \r\n");

    for (i = 0; i < SIZEOF_ARRAY(bufVCardField); i++)
    {
        /* Requesting vCard field */
        bufType = bufVCardField[i];

        /* Get information from vCard */
        ndefGetVCard(vCard, bufType, &bufSubType, &bufValue);

        if (bufValue.buffer != NULL)
        {
            ndefConstBuffer bufTypeTranslate;
            ndefMediaVCardTranslate(bufType, &bufTypeTranslate);

            /* Type */
            ndefBufferPrint(" ", &bufTypeTranslate, "");

            /* Subtype, if any */
            if (bufSubType.buffer != NULL)
            {
                ndefBufferPrint(" (", &bufSubType, ")");
            }

            /* Value */
            if (ST_BYTECMP(bufType->buffer, bufTypePHOTO.buffer, bufTypePHOTO.length) != 0)
            {
                ndefBufferPrint(": ", &bufValue, "\r\n");
            }
            else
            {
                platformLog("Photo: <Not displayed>\r\n");
            }
        }
    }

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefMediaWifiDump(const ndefType* wifi)
{
    ndefTypeWifi wifiConfig;

    if (wifi == NULL)
    {
        return ERR_PARAM;
    }

    if (wifi->id != NDEF_TYPE_MEDIA_WIFI)
    {
        return ERR_PARAM;
    }

    ndefGetWifi(wifi, &wifiConfig);

    platformLog(" Wifi config: \r\n");
    ndefBufferDump(" Network SSID:",       &wifiConfig.bufNetworkSSID, false);
    ndefBufferDump(" Network Key:",        &wifiConfig.bufNetworkKey, false);
    platformLog(" Authentication: %d\r\n",  wifiConfig.authentication);
    platformLog(" Encryption: %d\r\n",      wifiConfig.encryption);

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefRecordDumpType(const ndefRecord* record)
{
    ReturnCode err;
    ndefType   type;
    uint32_t i;

    err = ndefRecordToType(record, &type);
    if (err != ERR_NONE)
    {
        return err;
    }

    for (i = 0; i < SIZEOF_ARRAY(typeDumpTable); i++)
    {
        if (type.id == typeDumpTable[i].typeId)
        {
            /* Call the appropriate function to the matching record type */
            if (typeDumpTable[i].dump != NULL)
            {
                return typeDumpTable[i].dump(&type);
            }
        }
    }

    return ERR_NOT_IMPLEMENTED;
}


/*****************************************************************************/
static ReturnCode ndefBufferDumpLine(const uint8_t* buffer, const uint32_t offset, uint32_t lineLength, uint32_t remaining)
{
    uint32_t j;

    if (buffer == NULL)
    {
        return ERR_PARAM;
    }

    platformLog(" [%.4X] ", offset);

    /* Dump hex data */
    for (j = 0; j < remaining; j++)
    {
        platformLog("%.2X ", buffer[offset + j]);
    }
    /* Fill hex section if needed */
    for (j = 0; j < lineLength - remaining; j++)
    {
        platformLog("   ");
    }

    /* Dump characters */
    platformLog("|");
    for (j = 0; j < remaining; j++)
    {
        /* Dump only ASCII characters, otherwise replace with a '.' */
        platformLog("%2c", isPrintableASCII(&buffer[offset + j], 1) ? buffer[offset + j] : '.');
    }
    /* Fill ASCII section if needed */
    for (j = 0; j < lineLength - remaining; j++)
    {
        platformLog("  ");
    }
    platformLog(" |\r\n");

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefBufferDump(const char* string, const ndefConstBuffer* bufPayload, bool verbose)
{
    uint32_t bufferLengthMax = 32;
    const uint32_t lineLength = 8;
    uint32_t displayed;
    uint32_t remaining;
    uint32_t offset;

    if ( (string == NULL) || (bufPayload == NULL) )
    {
        return ERR_PARAM;
    }

    displayed = bufPayload->length;
    remaining = bufPayload->length;

    platformLog("%s (length %d)\r\n", string, bufPayload->length);
    if (bufPayload->buffer == NULL)
    {
        platformLog(" <No chunk payload buffer>\r\n");
        return ERR_NONE;
    }

    if (verbose == true)
    {
        bufferLengthMax = 256;
    }
    if (bufPayload->length > bufferLengthMax)
    {
        /* Truncate output */
        displayed = bufferLengthMax;
    }

    for (offset = 0; offset < displayed; offset += lineLength)
    {
        ndefBufferDumpLine(bufPayload->buffer, offset, lineLength, remaining > lineLength ? lineLength : remaining);
        remaining -= lineLength;
    }

    if (displayed < bufPayload->length)
    {
        platformLog(" ... (truncated)\r\n");
    }

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefBufferPrint(const char* prefix, const ndefConstBuffer* bufString, const char* suffix)
{
    uint32_t i;

    if ( (prefix == NULL) || (bufString == NULL) || (bufString->buffer == NULL) || (suffix  == NULL))
    {
        return ERR_PARAM;
    }

    platformLog("%s", prefix);
    for (i = 0; i < bufString->length; i++)
    {
        platformLog("%c", bufString->buffer[i]);
    }
    platformLog("%s", suffix);

    return ERR_NONE;
}


/*****************************************************************************/
ReturnCode ndefBuffer8Print(const char* prefix, const ndefConstBuffer8* bufString, const char* suffix)
{
    ndefConstBuffer buf;

    if (bufString == NULL)
    {
        return ERR_PARAM;
    }

    buf.buffer = bufString->buffer;
    buf.length = bufString->length;

    return ndefBufferPrint(prefix, &buf, suffix);
}
