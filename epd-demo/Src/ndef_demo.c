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
#include "utils.h"
#include "rfal_nfc.h"
#include "ndef_poller.h"
#include "ndef_t2t.h"
#include "ndef_t4t.h"
#include "ndef_t5t.h"
#include "ndef_message.h"
#include "ndef_types_rtd.h"
#include "ndef_dump.h"


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0  /*!< Demo State:  Not initialized */
#define DEMO_ST_START_DISCOVERY       1  /*!< Demo State:  Start Discovery */
#define DEMO_ST_DISCOVERY             2  /*!< Demo State:  Discovery       */

#define NDEF_DEMO_READ              0U   /*!< NDEF menu read               */
#define NDEF_DEMO_WRITE_MSG1        1U   /*!< NDEF menu write 1 record     */
#define NDEF_DEMO_WRITE_MSG2        2U   /*!< NDEF menu write 2 records    */
#define NDEF_DEMO_FORMAT_TAG        3U   /*!< NDEF menu format tag         */
#if NDEF_FEATURE_ALL
#define NDEF_DEMO_MAX_FEATURES      4U   /*!< Number of menu items         */
#else
#define NDEF_DEMO_MAX_FEATURES      1U   /*!< Number of menu items         */
#endif /* NDEF_FEATURE_ALL */
#define NDEF_WRITE_FORMAT_TIMEOUT   10000U /*!< When write or format mode is selected, demo returns back to read mode after a timeout */
#define NDEF_LED_BLINK_DURATION       250U /*!< Led blink duration         */ 

#define DEMO_RAW_MESSAGE_BUF_LEN      8192 /*!< Raw message buffer len     */

#define DEMO_ST_MANUFACTURER_ID      0x02U /*!< ST Manufacturer ID         */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

/* P2P communication data */
static uint8_t NFCID3[] = {0x01, 0xFE, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
static uint8_t GB[] = {0x46, 0x66, 0x6d, 0x01, 0x01, 0x11, 0x02, 0x02, 0x07, 0x80, 0x03, 0x02, 0x00, 0x03, 0x04, 0x01, 0x32, 0x07, 0x01, 0x03};
    
#if defined(ST25R3916) && defined(RFAL_FEATURE_LISTEN_MODE)
/* NFC-A CE config */
static uint8_t ceNFCA_NFCID[]     = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};                   /* NFCID / UID (7 bytes)                    */
static uint8_t ceNFCA_SENS_RES[]  = {0x44, 0x00};                                                 /* SENS_RES / ATQA                          */
static uint8_t ceNFCA_SEL_RES     = 0x20;                                                         /* SEL_RES / SAK                            */

/* NFC-F CE config */
static uint8_t ceNFCF_nfcid2[]     = {0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
static uint8_t ceNFCF_SC[]         = {0x12, 0xFC};
static uint8_t ceNFCF_SENSF_RES[]  = {0x01,                                                       /* SENSF_RES                                */
                                      0x02, 0xFE, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,             /* NFCID2                                   */
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00,             /* PAD0, PAD01, MRTIcheck, MRTIupdate, PAD2 */
                                      0x00, 0x00 };                                               /* RD                                       */
#endif /* RFAL_FEATURE_LISTEN_MODE */

/* P2P communication data */    
static uint8_t ndefLLCPSYMM[] = {0x00, 0x00};
static uint8_t ndefInit[] = {0x05, 0x20, 0x06, 0x0F, 0x75, 0x72, 0x6E, 0x3A, 0x6E, 0x66, 0x63, 0x3A, 0x73, 0x6E, 0x3A, 0x73, 0x6E, 0x65, 0x70, 0x02, 0x02, 0x07, 0x80, 0x05, 0x01, 0x02};
static const uint8_t ndefSnepPrefix[] = { 0x13, 0x20, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00 };
static const uint8_t URL[] = "st.com";
static ndefConstBuffer bufURL = { URL, sizeof(URL) - 1 };
static uint8_t ndefUriBuffer[255]; 

static uint8_t *ndefStates[] =
{
    (uint8_t *)"INVALID",
    (uint8_t *)"INITIALIZED",
    (uint8_t *)"READ/WRITE",
    (uint8_t *)"READ-ONLY"
};

static const uint8_t *ndefDemoFeatureDescription[NDEF_DEMO_MAX_FEATURES] =
{
    (uint8_t *)"1. Tap a tag to read its content",
#if NDEF_FEATURE_ALL
    (uint8_t *)"2. Present a tag to write a Text record",
    (uint8_t *)"3. Present a tag to write a URI record and an Android Application record",
    (uint8_t *)"4. Present an ST tag to format",
#endif /* NDEF_FEATURE_ALL */
};

#if NDEF_FEATURE_ALL
static uint8_t ndefURI[]          = "st.com";
static uint8_t ndefTEXT[]         = "Welcome to ST NDEF demo";
static uint8_t ndefTextLangCode[] = "en";

static uint8_t ndefAndroidPackName[] = "com.st.st25nfc";
#endif /* NDEF_FEATURE_ALL */
 
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */

static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

static ndefContext          ndefCtx;
static uint8_t              ndefDemoFeature     = NDEF_DEMO_READ;
static uint8_t              ndefDemoPrevFeature = 0xFF;
static bool                 verbose             = false;

static uint8_t              rawMessageBuf[DEMO_RAW_MESSAGE_BUF_LEN];

static uint32_t             timer;
static uint32_t             timerLed;
static bool                 ledOn;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/

static void demoNdef(rfalNfcDevice *nfcDevice);
static void ndefCCDump(ndefContext *ctx);
static void ndefDumpSysInfo(ndefContext *ctx);

#if NDEF_FEATURE_ALL
static bool ndefIsSTTag(ndefContext *ctx);
static void LedNotificationWriteDone(void);
#endif /* NDEF_FEATURE_ALL */

static void demoP2P( void );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );

static void ledsOn(void);
static void ledsOff(void);

/*!
 *****************************************************************************
 * \brief Check user button
 *
 *  This function check whethe the user button has been pressed
 *****************************************************************************
 */

static void checkUserButton(void)
{
    /* Check if USER button is pressed */
    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        ndefDemoFeature++;
        ndefDemoFeature %= NDEF_DEMO_MAX_FEATURES;

        ledsOff();
        ndefDemoPrevFeature = ndefDemoFeature;
        platformLog("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);
        /* Debounce button */
        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
        if( ndefDemoFeature != NDEF_DEMO_READ )
        {
            timer = platformTimerCreate(NDEF_WRITE_FORMAT_TIMEOUT);
            timerLed = platformTimerCreate(NDEF_LED_BLINK_DURATION);
        }
    }
}

/*!
 *****************************************************************************
 * \brief Show usage
 *
 *  This function displays usage information
 *****************************************************************************
 */
static void ndefShowDemoUsage()
{
#if NDEF_FEATURE_ALL
    uint32_t i;
    
    platformLog("Use the User button to cycle among the different modes:\r\n");
    for (i = 0; i < SIZEOF_ARRAY(ndefDemoFeatureDescription); i++)
    {
        platformLog("%s\r\n", ndefDemoFeatureDescription[i]);
    }
    platformLog("In Write or Format mode (menu 2, 3 or 4), the demo returns to Read mode (menu 1) if no tag detected after %d seconds\r\n\n", NDEF_WRITE_FORMAT_TIMEOUT/1000);
#endif /* NDEF_FEATURE_ALL */
}

/*!
 *****************************************************************************
 * \brief Demo Ini
 *
 *  This function Initializes the required layers for the demo
 *
 * \return true  : Initialization ok
 * \return false : Initialization failed
 *****************************************************************************
 */
bool demoIni( void )
{
    ReturnCode err;
    
#if defined(STM32L476xx)
    if( (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0)
    {
        verbose = true;
    }
#endif
    ndefShowDemoUsage();
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        discParam.compMode      = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit      = 1U;
        discParam.nfcfBR        = RFAL_BR_212;
        discParam.ap2pBR        = RFAL_BR_424;

        ST_MEMCPY( &discParam.nfcid3, NFCID3, sizeof(NFCID3) );
        ST_MEMCPY( &discParam.GB, GB, sizeof(GB) );
        discParam.GBLen         = sizeof(GB);

        discParam.notifyCb             = NULL;
        discParam.totalDuration        = 1000U;
        discParam.wakeupEnabled        = false;
        discParam.wakeupConfigDefault  = true;
        discParam.techs2Find           = ( RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_F | RFAL_NFC_POLL_TECH_V | RFAL_NFC_POLL_TECH_ST25TB );
#if defined(ST25R3911) || defined(ST25R3916)
        discParam.techs2Find   |= RFAL_NFC_POLL_TECH_AP2P;
#endif /* ST25R3911 || ST25R3916 */
        
        
#if defined(ST25R3916)
      
      /* Set configuration for NFC-A CE */
      ST_MEMCPY( discParam.lmConfigPA.SENS_RES, ceNFCA_SENS_RES, RFAL_LM_SENS_RES_LEN );                        /* Set SENS_RES / ATQA */
      ST_MEMCPY( discParam.lmConfigPA.nfcid, ceNFCA_NFCID, RFAL_NFCID2_LEN );                                   /* Set NFCID / UID */
      discParam.lmConfigPA.nfcidLen = RFAL_LM_NFCID_LEN_07;                                                     /* Set NFCID length to 7 bytes */
      discParam.lmConfigPA.SEL_RES  = ceNFCA_SEL_RES;                                                           /* Set SEL_RES / SAK */

      /* Set configuration for NFC-F CE */
      ST_MEMCPY( discParam.lmConfigPF.SC, ceNFCF_SC, RFAL_LM_SENSF_SC_LEN );                                    /* Set System Code */
      ST_MEMCPY( &ceNFCF_SENSF_RES[RFAL_NFCF_LENGTH_LEN], ceNFCF_nfcid2, RFAL_LM_SENSF_RES_LEN );               /* Load NFCID2 on SENSF_RES */
      ST_MEMCPY( discParam.lmConfigPF.SENSF_RES, ceNFCF_SENSF_RES, RFAL_LM_SENSF_RES_LEN );                     /* Set SENSF_RES / Poll Response */
      
      discParam.techs2Find |= ( RFAL_NFC_LISTEN_TECH_A | RFAL_NFC_LISTEN_TECH_F );
      
#endif /* ST25R3916 */

        state = DEMO_ST_START_DISCOVERY;
        return true;
    }
    return false;
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
    static rfalNfcDevice *nfcDevice;

    rfalNfcaSensRes       sensRes;
    rfalNfcaSelRes        selRes;
    
    rfalNfcbSensbRes      sensbRes;
    uint8_t               sensbResLen;
    
    uint8_t               devCnt = 0;
    rfalFeliCaPollRes     cardList[1];
    uint8_t               collisions = 0U;
    rfalNfcfSensfRes*     sensfRes;

    rfalNfcvInventoryRes  invRes;
    uint16_t              rcvdLen;
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */
    
    if( (ndefDemoFeature != NDEF_DEMO_READ) && (platformTimerIsExpired(timer)) )
    {
        platformLog("Timer expired, back to Read mode...\r\n");
        ndefDemoFeature = NDEF_DEMO_READ;
    }
    
    if( ndefDemoFeature != ndefDemoPrevFeature )
    {
        ndefDemoPrevFeature = ndefDemoFeature;
        platformLog("%s\r\n", ndefDemoFeatureDescription[ndefDemoFeature]);
    }
    
    if( ndefDemoFeature != NDEF_DEMO_READ )
    {
        if( platformTimerIsExpired(timerLed) )
        {
            timerLed = platformTimerCreate(NDEF_LED_BLINK_DURATION);
            ledOn = !ledOn;
        }
        if( ledOn )
        {
            ledsOn();
        }
        else
        {
            ledsOff();
        }
    }
    
    checkUserButton();
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:
            ledsOff();
    
            rfalNfcDeactivate( false );
            rfalNfcDiscover( &discParam );

            state = DEMO_ST_DISCOVERY;
            break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
                ledsOff();
                platformDelay(50);
                ndefDemoPrevFeature = 0xFF; /* Force the display of the prompt */
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCA:
                    
                        platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
                        switch( nfcDevice->dev.nfca.type )
                        {
                            case RFAL_NFCA_T1T:
                                platformLog("ISO14443A/Topaz (NFC-A T1T) TAG found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                rfalNfcaPollerSleep();
                                break;
                            
                            case RFAL_NFCA_T4T:
                                platformLog("NFCA Passive ISO-DEP device found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                demoNdef(nfcDevice);
                                rfalIsoDepDeselect(); 
                                break;
                            
                            case RFAL_NFCA_T4T_NFCDEP:
                            case RFAL_NFCA_NFCDEP:
                                platformLog("NFCA Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                demoP2P();
                                break;
                                
                            default:
                                platformLog("ISO14443A/NFC-A card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                                demoNdef(nfcDevice);
                                rfalNfcaPollerSleep();
                                break;
                        }
                        /* Loop until tag is removed from the field */
                        platformLog("Operation completed\r\nTag can be removed from the field\r\n");
                        rfalNfcaPollerInitialize();
                        while( rfalNfcaPollerCheckPresence(RFAL_14443A_SHORTFRAME_CMD_WUPA, &sensRes) == ERR_NONE )
                        {
                            if( ((nfcDevice->dev.nfca.type == RFAL_NFCA_T1T) && (!rfalNfcaIsSensResT1T(&sensRes ))) ||
                                ((nfcDevice->dev.nfca.type != RFAL_NFCA_T1T) && (rfalNfcaPollerSelect(nfcDevice->dev.nfca.nfcId1, nfcDevice->dev.nfca.nfcId1Len, &selRes) != ERR_NONE)) )
                            {
                                break;
                            }
                            rfalNfcaPollerSleep();
                            platformDelay(130);
                        }
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCB:
                        
                        platformLog("ISO14443B/NFC-B card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                    
                        if( rfalNfcbIsIsoDepSupported( &nfcDevice->dev.nfcb ) )
                        {
                            demoNdef(nfcDevice);
                            rfalIsoDepDeselect();
                        }
                        else
                        {
                            rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                        }
                        /* Loop until tag is removed from the field */
                        platformLog("Operation completed\r\nTag can be removed from the field\r\n");
                        rfalNfcbPollerInitialize();
                        while( rfalNfcbPollerCheckPresence(RFAL_NFCB_SENS_CMD_ALLB_REQ, RFAL_NFCB_SLOT_NUM_1, &sensbRes, &sensbResLen) == ERR_NONE )
                        {
                            if( ST_BYTECMP(sensbRes.nfcid0, nfcDevice->dev.nfcb.sensbRes.nfcid0, RFAL_NFCB_NFCID0_LEN) != 0 )
                            {
                                break;
                            }
                            rfalNfcbPollerSleep(nfcDevice->dev.nfcb.sensbRes.nfcid0);
                            platformDelay(130);
                        }
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCF:
                        
                        if( rfalNfcfIsNfcDepSupported( &nfcDevice->dev.nfcf ) )
                        {
                            platformLog("NFCF Passive P2P device found. NFCID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ) );
                            demoP2P();
                        }
                        else
                        {
                            platformLog("Felica/NFC-F card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                            demoNdef(nfcDevice);
                        }
                        
                        platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
                        /* Loop until tag is removed from the field */
                        platformLog("Operation completed\r\nTag can be removed from the field\r\n");
                        devCnt = 1;
                        rfalNfcfPollerInitialize( RFAL_BR_212 );
                        while (rfalNfcfPollerPoll( RFAL_FELICA_1_SLOT, RFAL_NFCF_SYSTEMCODE, RFAL_FELICA_POLL_RC_NO_REQUEST, cardList, &devCnt, &collisions ) == ERR_NONE)
                        {
                            /* Skip the length field byte */
                            sensfRes = (rfalNfcfSensfRes*)&((uint8_t *)cardList)[1];
                            if( ST_BYTECMP(sensfRes->NFCID2, nfcDevice->dev.nfcf.sensfRes.NFCID2, RFAL_NFCF_NFCID2_LEN) != 0 )
                            {
                                break;
                            }
                            platformDelay(130);
                        }
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];
                            
                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );   /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                 /* Reverse the UID for display purposes */
                            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));
                        
                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
                            
                            demoNdef(nfcDevice);

                            /* Loop until tag is removed from the field */
                            platformLog("Operation completed\r\nTag can be removed from the field\r\n");
                            rfalNfcvPollerInitialize();
                            while (rfalNfcvPollerInventory( RFAL_NFCV_NUM_SLOTS_1, RFAL_NFCV_UID_LEN * 8U, nfcDevice->dev.nfcv.InvRes.UID, &invRes, &rcvdLen) == ERR_NONE)
                            {
                                platformDelay(130);
                            }
                        }
                        break;
                        
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_ST25TB:
                        
                        platformLog("ST25TB card found. UID: %s\r\n", hex2Str( nfcDevice->nfcid, nfcDevice->nfcidLen ));
                        platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
                        break;
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_AP2P:
                        
                        platformLog("NFC Active P2P device found. NFCID3: %s\r\n", hex2Str(nfcDevice->nfcid, nfcDevice->nfcidLen));
                        platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
                    
                        demoP2P();
                        break;
                    
                    /*******************************************************************************/
                    default:
                        break;
                }
                
                rfalNfcDeactivate( false );
                platformDelay( 500 );
                state = DEMO_ST_START_DISCOVERY;
            }
            break;

        /*******************************************************************************/
        case DEMO_ST_NOTINIT:
        default:
            break;
    }
}


/*!
 *****************************************************************************
 * \brief Demo P2P Exchange
 *
 * Sends a NDEF URI record 'http://www.ST.com' via NFC-DEP (P2P) protocol.
 * 
 * This method sends a set of static predefined frames which tries to establish
 * a LLCP connection, followed by the NDEF record, and then keeps sending 
 * LLCP SYMM packets to maintain the connection.
 * 
 * 
 *****************************************************************************
 */
void demoP2P( void )
{
    uint16_t   *rxLen;
    uint8_t    *rxData;
    ReturnCode err;

    ndefBuffer  bufPayload;
    ndefMessage message;
    ndefRecord  record;
    ndefType    uri;

    platformLog(" Initalize device .. ");
    err = demoTransceiveBlocking( ndefInit, sizeof(ndefInit), &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.");
        return;
    }
    platformLog("succeeded.\r\n");
    
    err  = ndefRtdUri(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufURL);
    err |= ndefRtdUriToRecord(&uri, &record);

    err |= ndefMessageInit(&message);
    err |= ndefMessageAppend(&message, &record);  /* To get MB and ME bits set */

    /* Build the SNEP buffer made of the prefix, the length byte and the record */
    ST_MEMCPY(ndefUriBuffer, ndefSnepPrefix, sizeof(ndefSnepPrefix));

    /* Skip 1 byte for length byte */
    bufPayload.buffer = ndefUriBuffer + sizeof(ndefSnepPrefix) + 1;
    bufPayload.length = sizeof(ndefUriBuffer) - sizeof(ndefSnepPrefix);
    err |= ndefMessageEncode(&message, &bufPayload);

    ndefUriBuffer[sizeof(ndefSnepPrefix)] = bufPayload.length;

    bufPayload.buffer = ndefUriBuffer;
    bufPayload.length = sizeof(ndefSnepPrefix) + 1 + bufPayload.length;

    if( err != ERR_NONE )
    {
        platformLog("NDEF message creation failed\r\n", err);
        return;
    }

    ndefBufferDump("URL converted to SNEP:\r\n", (ndefConstBuffer*)&bufPayload, true);

    platformLog(" Push NDEF Uri: www.ST.com .. ");
    err = demoTransceiveBlocking(bufPayload.buffer, bufPayload.length, &rxData, &rxLen, RFAL_FWT_NONE);
    if( err != ERR_NONE )
    {
        platformLog("failed.");
        return;
    }
    platformLog("succeeded.\r\n");


    platformLog(" Device present, maintaining connection ");
    while(err == ERR_NONE) 
    {
        err = demoTransceiveBlocking( ndefLLCPSYMM, sizeof(ndefLLCPSYMM), &rxData, &rxLen, RFAL_FWT_NONE);
        platformLog(".");
        platformDelay(50);
    }
    platformLog("\r\n Device removed.\r\n");
}


/*!
 *****************************************************************************
 * \brief Demo Blocking Transceive 
 *
 * Helper function to send data in a blocking manner via the rfalNfc module 
 *  
 * \warning A protocol transceive handles long timeouts (several seconds), 
 * transmission errors and retransmissions which may lead to a long period of 
 * time where the MCU/CPU is blocked in this method.
 * This is a demo implementation, for a non-blocking usage example please 
 * refer to the Examples available with RFAL
 *
 * \param[in]  txBuf      : data to be transmitted
 * \param[in]  txBufSize  : size of the data to be transmited
 * \param[out] rxData     : location where the received data has been placed
 * \param[out] rcvLen     : number of data bytes received
 * \param[in]  fwt        : FWT to be used (only for RF frame interface, 
 *                                          otherwise use RFAL_FWT_NONE)
 *
 * 
 *  \return ERR_PARAM     : Invalid parameters
 *  \return ERR_TIMEOUT   : Timeout error
 *  \return ERR_FRAMING   : Framing error detected
 *  \return ERR_PROTO     : Protocol error detected
 *  \return ERR_NONE      : No error, activation successful
 * 
 *****************************************************************************
 */
ReturnCode demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxData, uint16_t **rcvLen, uint32_t fwt )
{
    ReturnCode err;
    
    err = rfalNfcDataExchangeStart( txBuf, txBufSize, rxData, rcvLen, fwt );
    if( err == ERR_NONE )
    {
        do{
            rfalNfcWorker();
            err = rfalNfcDataExchangeGetStatus();
        }
        while( err == ERR_BUSY );
    }
    return err;
}

static void demoNdef(rfalNfcDevice *pNfcDevice)
{
    ReturnCode       err;
    ndefMessage      message;
    uint32_t         rawMessageLen;
    ndefInfo         info;
    ndefBuffer       bufRawMessage;
    ndefConstBuffer  bufConstRawMessage;
 
#if NDEF_FEATURE_ALL 
    ndefRecord       record1;
    ndefRecord       record2;

    ndefType         text;
    ndefType         uri;
    ndefType         aar;

    ndefConstBuffer8 bufTextLangCode;
    ndefConstBuffer bufTextLangText;
    ndefConstBuffer bufUri;
    ndefConstBuffer bufAndroidPackName;
#endif /* NDEF_FEATURE_ALL */


    /*
     * Perform NDEF Context Initialization
     */
    err = ndefPollerContextInitialization(&ndefCtx, pNfcDevice);
    if( err != ERR_NONE )
    {
        platformLog("NDEF NOT DETECTED (ndefPollerContextInitialization returns %d)\r\n", err);
        return;
    }
    
    if( verbose & (pNfcDevice->type == RFAL_NFC_LISTEN_TYPE_NFCV) )
    {
        ndefDumpSysInfo(&ndefCtx);
    }

    /*
     * Perform NDEF Detect procedure
     */
    err = ndefPollerNdefDetect(&ndefCtx, &info);
    if( err != ERR_NONE )
    {
        platformLog("NDEF NOT DETECTED (ndefPollerNdefDetect returns %d)\r\n", err);
        if( ndefDemoFeature != NDEF_DEMO_FORMAT_TAG)
        {
            return;
        }
    }
    else
    {
        platformLog("%s NDEF detected.\r\n", ndefStates[info.state]);
        ndefCCDump(&ndefCtx);

        if( verbose )
        {
            platformLog("NDEF Len: %d, Offset=%d\r\n", ndefCtx.messageLen, ndefCtx.messageOffset);
        }
    }

    switch( ndefDemoFeature )
    {
        /*
         * Demonstrate how to read the NDEF message from the Tag
         */
        case NDEF_DEMO_READ:
            if( info.state == NDEF_STATE_INITIALIZED )
            {
                /* Nothing to read... */
                return;
            }
            err = ndefPollerReadRawMessage(&ndefCtx, rawMessageBuf, sizeof(rawMessageBuf), &rawMessageLen);
            if( err != ERR_NONE )
            {
                platformLog("NDEF message cannot be read (ndefPollerReadRawMessage returns %d)\r\n", err);
                return;
            }
            if( verbose )
            {
                bufRawMessage.buffer = rawMessageBuf;
                bufRawMessage.length = rawMessageLen;
                ndefBufferDump(" NDEF Content", (ndefConstBuffer*)&bufRawMessage, verbose);
            }
            bufConstRawMessage.buffer = rawMessageBuf;
            bufConstRawMessage.length = rawMessageLen;
            err = ndefMessageDecode(&bufConstRawMessage, &message);
            if( err != ERR_NONE )
            {
                platformLog("NDEF message cannot be decoded (ndefMessageDecode  returns %d)\r\n", err);
                return;
            }
            err = ndefMessageDump(&message, verbose);
            if( err != ERR_NONE )
            {
                platformLog("NDEF message cannot be displayed (ndefMessageDump returns %d)\r\n", err);
                return;
            }
            break;

#if NDEF_FEATURE_ALL 
        /*
         * Demonstrate how to encode a text record and write the message to the tag
         */
        case NDEF_DEMO_WRITE_MSG1:
            ndefDemoFeature = NDEF_DEMO_READ; /* returns to READ mode after write */
            err  = ndefMessageInit(&message); /* Initialize message structure */
            bufTextLangCode.buffer = ndefTextLangCode;
            bufTextLangCode.length = strlen((char *)ndefTextLangCode);

            bufTextLangText.buffer = ndefTEXT;
            bufTextLangText.length = strlen((char *)ndefTEXT);

            err |= ndefRtdText(&text, TEXT_ENCODING_UTF8, &bufTextLangCode, &bufTextLangText); /* Initialize Text type structure */
            err |= ndefRtdTextToRecord(&text, &record1); /* Encode Text Record */
            err |= ndefMessageAppend(&message, &record1); /* Append Text record to message */
            if( err != ERR_NONE )
            {
                platformLog("Message creation failed\r\n", err);
                return;
            }
            err = ndefPollerWriteMessage(&ndefCtx, &message); /* Write message */
            if( err != ERR_NONE )
            {
                platformLog("Message cannot be written (ndefPollerWriteMessage return %d)\r\n", err);
                return;
            }
            platformLog("Wrote 1 record to the Tag\r\n");
            if( verbose )
            {
                /* Dump raw message */
                bufRawMessage.buffer = rawMessageBuf;
                bufRawMessage.length = sizeof(rawMessageBuf);
                err = ndefMessageEncode(&message, &bufRawMessage);
                if( err == ERR_NONE )
                {
                    ndefBufferDump("Raw message", (ndefConstBuffer*)&bufRawMessage, verbose);
                }
            }
            LedNotificationWriteDone();
            break;

        /*
         * Demonstrate how to encode a URI record and a AAR record, how to encode the message to a raw buffer and then how to write the raw buffer
         */
        case NDEF_DEMO_WRITE_MSG2:
            ndefDemoFeature = NDEF_DEMO_READ;  /* returns to READ mode after write */
            err  = ndefMessageInit(&message);  /* Initialize message structure */
            bufUri.buffer = ndefURI;
            bufUri.length = strlen((char *)ndefURI);
            err |= ndefRtdUri(&uri, NDEF_URI_PREFIX_HTTP_WWW, &bufUri); /* Initialize URI type structure */
            err |= ndefRtdUriToRecord(&uri, &record1); /* Encode URI Record */

            bufAndroidPackName.buffer = ndefAndroidPackName;
            bufAndroidPackName.length = sizeof(ndefAndroidPackName) - 1U;
            err |= ndefRtdAar(&aar, &bufAndroidPackName); /* Initialize AAR type structure */
            err |= ndefRtdAarToRecord(&aar, &record2); /* Encode AAR record */

            err |= ndefMessageAppend(&message, &record1); /* Append URI to message */
            err |= ndefMessageAppend(&message, &record2); /* Append AAR to message (record #2 is an example of preformatted record) */

            bufRawMessage.buffer = rawMessageBuf;
            bufRawMessage.length = sizeof(rawMessageBuf);
            err |= ndefMessageEncode(&message, &bufRawMessage); /* Encode the message to the raw buffer */
            if( err != ERR_NONE )
            {
                platformLog("Raw message creation failed\r\n", err);
                return;
            }
            err = ndefPollerWriteRawMessage(&ndefCtx, bufRawMessage.buffer, bufRawMessage.length);
            if( err != ERR_NONE )
            {
                platformLog("Message cannot be written (ndefPollerWriteRawMessage return %d)\r\n", err);
                return;
            }
            platformLog("Wrote 2 records to the Tag\r\n");
            if( verbose )
            {
                /* Dump raw message */
                ndefBufferDump("Raw message", (ndefConstBuffer*)&bufRawMessage, verbose);
            }
            LedNotificationWriteDone();
            break;

        /*
         * Demonstrate how to format a Tag
         */
        case NDEF_DEMO_FORMAT_TAG:
            ndefDemoFeature = NDEF_DEMO_READ;
            if( !ndefIsSTTag(&ndefCtx) )
            {
                platformLog("Manufacturer ID not found or not an ST tag. Format aborted \r\n");
                return;
            }
            platformLog("Formatting Tag...\r\n");
            /* Format Tag */
            err = ndefPollerTagFormat(&ndefCtx, NULL, 0);
            if( err != ERR_NONE )
            {
                platformLog("Tag cannot be formatted (ndefPollerTagFormat returns %d)\r\n", err);
                return;
            }
            platformLog("Tag formatted\r\n");
            LedNotificationWriteDone();
            break;
#endif /* NDEF_FEATURE_ALL */

        default:
            ndefDemoFeature = NDEF_DEMO_READ;
            break;     
    }
    return;
}

static void ndefT2TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;

    platformLog(" * Magic: %2.2Xh Version: %d.%d Size: %d (%d bytes) \r\n * readAccess: %2.2xh writeAccess: %2.2xh \r\n", ctx->cc.t2t.magicNumber, ctx->cc.t2t.majorVersion, ctx->cc.t2t.minorVersion, ctx->cc.t2t.size, ctx->cc.t2t.size * 8U, ctx->cc.t2t.readAccess, ctx->cc.t2t.writeAccess);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = 4;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
  
}

static void ndefT3TAIBDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;

    platformLog(" * Version: %d.%d Size: %d (%d bytes) NbR: %d NbW: %d\r\n * WriteFlag: %2.2xh RWFlag: %2.2xh \r\n", ctx->cc.t3t.majorVersion, ctx->cc.t3t.minorVersion, ctx->cc.t3t.nMaxB, ctx->cc.t3t.nMaxB * 16U, ctx->cc.t3t.nbR, ctx->cc.t3t.nbW, ctx->cc.t3t.writeFlag, ctx->cc.t3t.rwFlag);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = 16;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}

static void ndefT4TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;
    
    platformLog(" * CCLEN: %d T4T_VNo: %xh MLe: %d MLc: %d FileId: %2.2x%2.2xh FileSize: %d\r\n * readAccess: %2.2xh writeAccess: %2.2xh\r\n", ctx->cc.t4t.ccLen, ctx->cc.t4t.vNo, ctx->cc.t4t.mLe, ctx->cc.t4t.mLc, ctx->cc.t4t.fileId[0], ctx->cc.t4t.fileId[1],ctx->cc.t4t.fileSize, ctx->cc.t4t.readAccess, ctx->cc.t4t.writeAccess);
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = ctx->cc.t4t.ccLen;
    ndefBufferDump(" CC File Raw Data", &bufCcBuf, verbose);
}

static void ndefT5TCCDump(ndefContext *ctx)
{
    ndefConstBuffer bufCcBuf;
    
    platformLog(" * Block Length: %d\r\n", ctx->subCtx.t5t.blockLen);
    platformLog(" * %d bytes CC\r\n * Magic: %2.2Xh Version: %d.%d MLEN: %d (%d bytes) \r\n * readAccess: %2.2xh writeAccess: %2.2xh \r\n", ctx->cc.t5t.ccLen, ctx->cc.t5t.magicNumber, ctx->cc.t5t.majorVersion, ctx->cc.t5t.minorVersion, ctx->cc.t5t.memoryLen, ctx->cc.t5t.memoryLen * 8U, ctx->cc.t5t.readAccess, ctx->cc.t5t.writeAccess);
    platformLog(" * [%c] Special Frame\r\n",       ctx->cc.t5t.specialFrame ?      'X' : ' ');
    platformLog(" * [%c] Multiple block Read\r\n", ctx->cc.t5t.multipleBlockRead ? 'X' : ' ');
    platformLog(" * [%c] Lock Block\r\n",          ctx->cc.t5t.lockBlock ?         'X' : ' ');
    bufCcBuf.buffer = ctx->ccBuf;
    bufCcBuf.length = ctx->cc.t5t.ccLen;
    ndefBufferDump(" CC Raw Data", &bufCcBuf, verbose);
}

static void ndefCCDump(ndefContext *ctx)
{
    if( (ctx == NULL) || !verbose)
    {
        return;
    }
    platformLog("%s", (ctx->device.type ==  RFAL_NFC_LISTEN_TYPE_NFCF) ? "NDEF Attribute Information Block\r\n" : "NDEF Capability Container\r\n");
    switch( ctx->device.type )
    {
        case RFAL_NFC_LISTEN_TYPE_NFCA:
            switch( ctx->device.dev.nfca.type )
            {            
                case RFAL_NFCA_T2T:
                    ndefT2TCCDump(ctx);
                    break;
                case RFAL_NFCA_T4T:
                    ndefT4TCCDump(ctx);
                    break;
                default:
                    break;
            }
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            ndefT4TCCDump(ctx);
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCF:
            ndefT3TAIBDump(ctx);
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCV:
            ndefT5TCCDump(ctx);
            break;
        default:
            break;
    }
}

static void ndefDumpSysInfo(ndefContext *ctx)
{
    ndefSystemInformation *sysInfo;

    if( (ctx == NULL) || !verbose)
    {
        return;
    }
    
    if( !ctx->subCtx.t5t.sysInfoSupported )
    {
        return;
    }
    
    sysInfo = &ctx->subCtx.t5t.sysInfo;
    platformLog("System Information\r\n");
    platformLog(" * %d byte(s) memory addressing\r\n", ndefT5TSysInfoMOIValue(sysInfo->infoFlags) + 1);
    if( ndefT5TSysInfoDFSIDPresent(sysInfo->infoFlags) )
    {
        platformLog(" * DFSID=%2.2Xh\r\n", sysInfo->DFSID);
    }
    if( ndefT5TSysInfoAFIPresent(sysInfo->infoFlags) )
    {
        platformLog(" * AFI=%2.2Xh\r\n", sysInfo->AFI);
    }
    if( ndefT5TSysInfoMemSizePresent(sysInfo->infoFlags) )
    {
        platformLog(" * %d blocks, %d bytes per block\r\n", sysInfo->numberOfBlock, sysInfo->blockSize);
    }
    if( ndefT5TSysInfoICRefPresent(sysInfo->infoFlags) )
    {
        platformLog(" * ICRef=%2.2xh\r\n", sysInfo->ICRef);
    }
    if( ndefT5TSysInfoCmdListPresent(sysInfo->infoFlags) )
    {
        platformLog(" * [%c] ReadSingleBlock                \r\n", ndefT5TSysInfoReadSingleBlockSupported(sysInfo->supportedCmd)                 ? 'X' : ' ');               
        platformLog(" * [%c] WriteSingleBlock               \r\n", ndefT5TSysInfoWriteSingleBlockSupported(sysInfo->supportedCmd)                ? 'X' : ' ');
        platformLog(" * [%c] LockSingleBlock                \r\n", ndefT5TSysInfoLockSingleBlockSupported(sysInfo->supportedCmd)                 ? 'X' : ' ');
        platformLog(" * [%c] ReadMultipleBlocks             \r\n", ndefT5TSysInfoReadMultipleBlocksSupported(sysInfo->supportedCmd)              ? 'X' : ' ');
        platformLog(" * [%c] WriteMultipleBlocks            \r\n", ndefT5TSysInfoWriteMultipleBlocksSupported(sysInfo->supportedCmd)             ? 'X' : ' ');
        platformLog(" * [%c] Select                         \r\n", ndefT5TSysInfoSelectSupported(sysInfo->supportedCmd)                          ? 'X' : ' ');
        platformLog(" * [%c] ResetToReady                   \r\n", ndefT5TSysInfoResetToReadySupported(sysInfo->supportedCmd)                    ? 'X' : ' ');
        platformLog(" * [%c] GetMultipleBlockSecStatus      \r\n", ndefT5TSysInfoGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd)       ? 'X' : ' ');
        platformLog(" * [%c] WriteAFI                       \r\n", ndefT5TSysInfoWriteAFISupported(sysInfo->supportedCmd)                        ? 'X' : ' ');
        platformLog(" * [%c] LockAFI                        \r\n", ndefT5TSysInfoLockAFISupported(sysInfo->supportedCmd)                         ? 'X' : ' ');
        platformLog(" * [%c] WriteDSFID                     \r\n", ndefT5TSysInfoWriteDSFIDSupported(sysInfo->supportedCmd)                      ? 'X' : ' ');
        platformLog(" * [%c] LockDSFID                      \r\n", ndefT5TSysInfoLockDSFIDSupported(sysInfo->supportedCmd)                       ? 'X' : ' ');
        platformLog(" * [%c] GetSystemInformation           \r\n", ndefT5TSysInfoGetSystemInformationSupported(sysInfo->supportedCmd)            ? 'X' : ' ');
        platformLog(" * [%c] CustomCmds                     \r\n", ndefT5TSysInfoCustomCmdsSupported(sysInfo->supportedCmd)                      ? 'X' : ' ');
        platformLog(" * [%c] FastReadMultipleBlocks         \r\n", ndefT5TSysInfoFastReadMultipleBlocksSupported(sysInfo->supportedCmd)          ? 'X' : ' ');
        platformLog(" * [%c] ExtReadSingleBlock             \r\n", ndefT5TSysInfoExtReadSingleBlockSupported(sysInfo->supportedCmd)              ? 'X' : ' '); 
        platformLog(" * [%c] ExtWriteSingleBlock            \r\n", ndefT5TSysInfoExtWriteSingleBlockSupported(sysInfo->supportedCmd)             ? 'X' : ' ');
        platformLog(" * [%c] ExtLockSingleBlock             \r\n", ndefT5TSysInfoExtLockSingleBlockSupported(sysInfo->supportedCmd)              ? 'X' : ' ');
        platformLog(" * [%c] ExtReadMultipleBlocks          \r\n", ndefT5TSysInfoExtReadMultipleBlocksSupported(sysInfo->supportedCmd)           ? 'X' : ' ');
        platformLog(" * [%c] ExtWriteMultipleBlocks         \r\n", ndefT5TSysInfoExtWriteMultipleBlocksSupported(sysInfo->supportedCmd)          ? 'X' : ' ');
        platformLog(" * [%c] ExtGetMultipleBlockSecStatus   \r\n", ndefT5TSysInfoExtGetMultipleBlockSecStatusSupported(sysInfo->supportedCmd)    ? 'X' : ' ');
        platformLog(" * [%c] FastExtendedReadMultipleBlocks \r\n", ndefT5TSysInfoFastExtendedReadMultipleBlocksSupported(sysInfo->supportedCmd)  ? 'X' : ' ');
    }
    return;
}

#if NDEF_FEATURE_ALL
static bool ndefIsSTTag(ndefContext *ctx)
{
    bool ret = false;

#if defined(STM32L476xx) //FIXME: temporary code to enable format tag from any manufacturer
  if( (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) != 0)
  {
    ret = true;
  }
#endif
    if( ctx == NULL )
    {   
        return ret;
    }
    switch (ctx->device.type)
    {
        case RFAL_NFC_LISTEN_TYPE_NFCA:
            if( (ctx->device.dev.nfca.nfcId1Len != 4) && (ctx->device.dev.nfca.nfcId1[0] == 0x02 ) )
            {  
                ret = true;
            }
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCF:
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCB:
            break;
        case RFAL_NFC_LISTEN_TYPE_NFCV:
            if( ctx->device.dev.nfcv.InvRes.UID[6] == 0x02 )
            {  
                ret = true;
            }
            break;
        default:
            break;
    }
    return (ret);
}
#endif /* NDEF_FEATURE_ALL */

#if NDEF_FEATURE_ALL
static void LedNotificationWriteDone(void)
{
    uint32_t i;

    for (i = 0; i < 3; i++)
    {
        ledsOn();
        platformDelay(100);

        ledsOff();
        platformDelay(100);
    }
}

static void ledsOn(void)
{
    platformLedOn(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    platformLedOn(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
    platformLedOn(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
    platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
    platformLedOn(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
    platformLedOn(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
}

static void ledsOff(void)
{
    platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
    platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
    platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
    platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
    platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
    platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
}

#endif /* NDEF_FEATURE_ALL */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
