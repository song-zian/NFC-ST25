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
/*  \file ndef_example_write.c
 *
 *  \example ndef_example_write.c
 *
 *  \author 
 *
 *  \brief NDEF Write example code
 *
 *  This demo shows how to poll for several types of NFC cards/devices and how 
 *  to Encode and Write an NDEF message using the RFAL and NDEF libraries.
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


/*!
 *****************************************************************************
 * \brief ndefExampleWrite
 *
 *  This function performs the various NFC activities up to the NDEF tag writing
 *****************************************************************************
 */
void ndefExampleWrite( void )
{
    ReturnCode      err;
    ndefConstBuffer bufUri;
    ndefType        uri;
    ndefRecord      record;
    ndefMessage     message;
    static uint8_t  ndefURI[] = "st.com";

    /*
     * Message creation
     */
    err  = ndefMessageInit(&message);  /* Initialize message structure */
    bufUri.buffer = ndefURI;
    bufUri.length = strlen((char *)ndefURI);
    err |= ndefRtdUri(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufUri); /* Initialize URI type structure */
    err |= ndefRtdUriToRecord(&uri, &record); /* Encode URI Record */
    err |= ndefMessageAppend(&message, &record); /* Append URI to message */
    if( err != ERR_NONE )
    {
        platformLog("Message creation failed\r\n", err);
        return;
    }

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
     * Write loop
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
             * Perform NDEF write procedure
             */
            err = ndefPollerWriteMessage(&ndefCtx, &message);
            if( err != ERR_NONE )
            {
                platformLog("NDEF message cannot be written (ndefPollerReadRawMessage returns %d)\r\n", err);
                return;
            }

            platformLog("NDEF Write successful\r\n");
            return;
        }
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
