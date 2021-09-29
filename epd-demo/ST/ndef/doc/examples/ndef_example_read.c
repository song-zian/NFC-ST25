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
/*  \file ndef_example_read.c
 *
 *  \example ndef_example_read.c
 *
 *  \author 
 *
 *  \brief NDEF Read example code
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to Read NDEF using the RFAL and NDEF libraries.
 *
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */

#include "utils.h"
#include "rfal_nfc.h"
#include "ndef_poller.h"
#include "ndef_message.h"
#include "ndef_types_rtd.h"
#include "ndef_types_mime.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define DEMO_RAW_MESSAGE_BUF_LEN      8192 /*!< raw message buffer size */


/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

static rfalNfcDevice        *nfcDevice;
static rfalNfcDiscoverParam discParam =
{
    .compMode            = RFAL_COMPLIANCE_MODE_NFC,
    .devLimit            = 1U,
    .nfcfBR              = RFAL_BR_212,
    .ap2pBR              = RFAL_BR_424,
    .nfcid3              = NULL,
    .GB                  = NULL,
    .GBLen               = 0,
    .notifyCb            = NULL,
    .totalDuration       = 1000U,
    .wakeupEnabled       = false,
    .wakeupConfigDefault = true,
    .techs2Find          = ( RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_F | RFAL_NFC_POLL_TECH_V )
};

static ndefContext          ndefCtx;
static uint8_t              rawMessageBuf[DEMO_RAW_MESSAGE_BUF_LEN];

/*
 ******************************************************************************
 * LOCAL FUNCTION PROTOTYPES
 ******************************************************************************
 */
void ndefExampleParseMessage(uint8_t* rawMsgBuf, uint32_t rawMsgLen);
void ndefExampleParseRecord(ndefRecord *record);
void ndefExamplePrintString(const uint8_t* str, uint32_t strLen);

/*!
 *****************************************************************************
 * \brief ndefExampleRead
 *
 *  This function performs the various NFC activities up to the NDEF tag reading
 *****************************************************************************
 */
void ndefExampleRead( void )
{
    ReturnCode err;
    uint32_t   rawMessageLen;

    /*
     * RFAL Init
     */
    err = rfalNfcInitialize();
    if( err != ERR_NONE )
    {
        platformLog("rfalNfcInitialize return %d\r\n", err);
        return;
    }

    rfalNfcDeactivate( false );
    rfalNfcDiscover( &discParam );

    /*
     * Read loop
     */
    while (1)
    {
        rfalNfcWorker();
        if( rfalNfcIsDevActivated(rfalNfcGetState()) )
        {
            /*
             * Retrieve NFC device
             */
            rfalNfcGetActiveDevice(&nfcDevice);

            /*
             * Perform NDEF Context Initialization
             */
            err = ndefPollerContextInitialization(&ndefCtx, nfcDevice);
            if( err != ERR_NONE )
            {
                platformLog("NDEF NOT DETECTED (ndefPollerContextInitialization returns %d)\r\n", err);
                return;
            }
    
            /*
             * Perform NDEF Detect procedure
             */
            err = ndefPollerNdefDetect(&ndefCtx, NULL);
            if( err != ERR_NONE )
            {
                platformLog("NDEF NOT DETECTED (ndefPollerNdefDetect returns %d)\r\n", err);
                return;
            }

            /*
             * Perform NDEF read procedure
             */
            err = ndefPollerReadRawMessage(&ndefCtx, rawMessageBuf, sizeof(rawMessageBuf), &rawMessageLen);
            if( err != ERR_NONE )
            {
                platformLog("NDEF message cannot be read (ndefPollerReadRawMessage returns %d)\r\n", err);
                return;
            }

            platformLog("NDEF Read successful\r\n");
            
            /*
             * Parse message content
             */
            ndefExampleParseMessage(rawMessageBuf, rawMessageLen);
            
            return;
        }
    }
}

/*!
 *****************************************************************************
 * \brief ndefExampleParseMessage
 *
 *  This function parses the NDEF message
 *****************************************************************************
 */
void ndefExampleParseMessage(uint8_t* rawMsgBuf, uint32_t rawMsgLen)
{
    ReturnCode       err;
    ndefConstBuffer  bufRawMessage;
    ndefMessage      message;
    ndefRecord*      record;

    bufRawMessage.buffer = rawMsgBuf;
    bufRawMessage.length = rawMsgLen;

    err = ndefMessageDecode(&bufRawMessage, &message);
    if (err != ERR_NONE)
    {
        return;
    }

    record = ndefMessageGetFirstRecord(&message);

    while (record != NULL)
    {
        ndefExampleParseRecord(record);
        record = ndefMessageGetNextRecord(record);
    }
}

/*!
 *****************************************************************************
 * \brief ndefExamplePrintString
 *
 *  This function prints a String of a given Length
 *****************************************************************************
 */
void ndefExamplePrintString(const uint8_t* str, uint32_t strLen)
{
    uint32_t i;
    const uint8_t* c;

    i = strLen;
    c = str;

    while (i-- > 0U)
    {
        platformLog("%c", *c);
        c++;
    }
    platformLog("\r\n");
}

/*!
 *****************************************************************************
 * \brief ndefExampleParseRecord
 *
 *  This function parses a given record
 *****************************************************************************
 */
void ndefExampleParseRecord(ndefRecord* record)
{
    ReturnCode       err;
    ndefType         type;

    err = ndefRecordToType(record, &type);
    if (err != ERR_NONE)
    {
        return;
    }

    switch (type.id)
    {
        case NDEF_TYPE_EMPTY:
             platformLog(" * Empty record\r\n");
             break;
        case NDEF_TYPE_RTD_TEXT:
             platformLog(" * TEXT record: ");
             ndefExamplePrintString(type.data.text.bufSentence.buffer, type.data.text.bufSentence.length);
             break;
        case NDEF_TYPE_RTD_URI:
             platformLog(" * URI record: ");
             ndefExamplePrintString(type.data.uri.bufUriString.buffer, type.data.uri.bufUriString.length);
             break;
        case NDEF_TYPE_RTD_AAR:
             platformLog(" * AAR record: ");
             ndefExamplePrintString(type.data.aar.bufPayload.buffer, type.data.aar.bufPayload.length);
             break;
        case NDEF_TYPE_RTD_DEVICE_INFO:
            platformLog(" * Device Info record\r\n");
            break;
        case NDEF_TYPE_MEDIA_VCARD:
            platformLog(" * vCard record\r\n");
            break;
        case NDEF_TYPE_MEDIA_WIFI:
            platformLog(" * WIFI record\r\n");
            break;
        default:
             platformLog(" * Other record\r\n");
             break;
     }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
