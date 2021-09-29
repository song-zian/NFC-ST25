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
#include "rfal_st25xv.h"


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT               0     /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY       1     /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY             2     /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN           4     /*!< NFCV Block len                      */
#define RFAL_NFCV_CMD_GET_BLK_SECURITY_STATUS    0x2CU  /*!< Get System Information command                               */
#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */
#define DEMO_NFCV_WRITE_TAG           false /*!< NFCV demonstrate Write Single Block */
#define DEMO_NFCV_LOCK_BLOCK          true //CL/*!< NFCV demonstrate Lock Single Block */
/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */


  
/*
 ******************************************************************************
 * LOCAL VARIABLES
 ******************************************************************************
 */
static rfalNfcDiscoverParam discParam;
static uint8_t              state = DEMO_ST_NOTINIT;

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/


static void demoNfcv( rfalNfcvListenDevice *nfcvDev );

static void demoNotif( rfalNfcState st );
ReturnCode  demoTransceiveBlocking( uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt );
ReturnCode rfalNfcvPollerGetBlockSecurityStatus( uint8_t flags, const uint8_t* uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t* rxBuf, uint16_t rxBufLen, uint16_t *rcvLen );


/*!
 *****************************************************************************
 * \brief Demo Notification
 *
 *  This function receives the event notifications from RFAL
 *****************************************************************************
 */
static void demoNotif( rfalNfcState st )
{
    uint8_t       devCnt;
    rfalNfcDevice *dev;
    
    
    if( st == RFAL_NFC_STATE_WAKEUP_MODE )
    {
        platformLog("Wake Up mode started \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_TECHDETECT )
    {
        platformLog("Wake Up mode terminated. Polling for devices \r\n");
    }
    else if( st == RFAL_NFC_STATE_POLL_SELECT )
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound( &dev, &devCnt );
        rfalNfcSelect( 0 );
        
        platformLog("Multiple Tags detected: %d \r\n", devCnt);
    }
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
    
    err = rfalNfcInitialize();
    if( err == ERR_NONE )
    {
        discParam.compMode      = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit      = 1U;


        discParam.notifyCb             = demoNotif;
        discParam.totalDuration        = 1000U;
        discParam.wakeupEnabled        = false;
        discParam.wakeupConfigDefault  = true;
        discParam.techs2Find           = (  RFAL_NFC_POLL_TECH_V );
        

	
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
    
    rfalNfcWorker();                                    /* Run RFAL worker periodically */

    /*******************************************************************************/
    /* Check if USER button is pressed */
    if( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled;    /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                       /* restart loop          */
        platformLog("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON": "OFF");

        /* Debounce button */
        while( platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN) );
    }
  
    
    switch( state )
    {
        /*******************************************************************************/
        case DEMO_ST_START_DISCOVERY:

          platformLedOff(PLATFORM_LED_A_PORT, PLATFORM_LED_A_PIN);
          platformLedOff(PLATFORM_LED_B_PORT, PLATFORM_LED_B_PIN);
          platformLedOff(PLATFORM_LED_F_PORT, PLATFORM_LED_F_PIN);
          platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
          platformLedOff(PLATFORM_LED_AP2P_PORT, PLATFORM_LED_AP2P_PIN);
          platformLedOff(PLATFORM_LED_FIELD_PORT, PLATFORM_LED_FIELD_PIN);
          
          rfalNfcDeactivate( false );
          rfalNfcDiscover( &discParam );
          
          state = DEMO_ST_DISCOVERY;
          break;

        /*******************************************************************************/
        case DEMO_ST_DISCOVERY:
        
            if( rfalNfcIsDevActivated( rfalNfcGetState() ) )
            {
                rfalNfcGetActiveDevice( &nfcDevice );
                
                switch( nfcDevice->type )
                {
                    /*******************************************************************************/

                    /*******************************************************************************/
          
                    
                    /*******************************************************************************/
                    case RFAL_NFC_LISTEN_TYPE_NFCV:
                        {
                            uint8_t devUID[RFAL_NFCV_UID_LEN];
                            
                            ST_MEMCPY( devUID, nfcDevice->nfcid, nfcDevice->nfcidLen );   /* Copy the UID into local var */
                            REVERSE_BYTES( devUID, RFAL_NFCV_UID_LEN );                 /* Reverse the UID for display purposes */
                            platformLog("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));
                        
                            platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);
                            
                            demoNfcv( &nfcDevice->dev.nfcv );
                        }
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
 * \brief Demo NFC-V Exchange
 *
 * Example how to exchange read and write blocks on a NFC-V tag
 * 
 *****************************************************************************
 */
static void demoNfcv( rfalNfcvListenDevice *nfcvDev )
{
    ReturnCode            err;
    uint8_t               regNum = 0;
		uint8_t               regVal;
    uint8_t *             uid; 
		uint8_t 							pwdNum=0;
		uint8_t 							pwd[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //CL: for ST25DV, pwd is 8 bytes
             
    uid = nfcvDev->InvRes.UID;
    
		err = rfalST25xVPollerReadConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, uid, regNum, &regVal );
		platformLog(" Read reg 0x%x: %s 0x%x\r\n",regNum,(err != ERR_NONE) ? "FAIL": " ",regVal);
	
	
		err = rfalST25xVPollerPresentPassword( RFAL_NFCV_REQ_FLAG_DEFAULT, uid, pwdNum, pwd, sizeof(pwd)); //CL: before changing the static register, need to present firstly the PWD.
		if(err==ERR_NONE)
		{
			regVal = 0x08;
			err = rfalST25xVPollerWriteConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, uid, regNum, regVal );
			platformLog(" Write reg 0x%x: %s 0x%x\r\n",regNum,(err != ERR_NONE) ? "FAIL": " ",regVal);	
			
			err = rfalST25xVPollerReadConfiguration( RFAL_NFCV_REQ_FLAG_DEFAULT, uid, regNum, &regVal );
			platformLog(" Read reg 0x%x: %s 0x%x\r\n",regNum,(err != ERR_NONE) ? "FAIL": " ",regVal);	
		}
		else
		{
			platformLog("Error code %d!\n",err);	
		}
}


/*! 
 *****************************************************************************
 * \brief  NFC-V Get Multiple Block Security Status request format
 *  
 * Sends Get Multiple Block Security Status request command  
 *
 * \param[in]  flags          : Flags to be used: Sub-carrier; Data_rate; Option
 *                              for NFC-Forum use: RFAL_NFCV_REQ_FLAG_DEFAULT
 * \param[in]  uid            : UID of the device to be put to be read
 *                               if not provided Select mode will be used 
 * \param[in]  requestField   : Get System info parameter request field
 * \param[out] rxBuf          : buffer to store response (also with RES_FLAGS)
 * \param[in]  rxBufLen       : length of rxBuf
 * \param[out] rcvLen         : number of bytes received
 *  
 * \return ERR_WRONG_STATE    : RFAL not initialized or incorrect mode
 * \return ERR_PARAM          : Invalid parameters
 * \return ERR_IO             : Generic internal error 
 * \return ERR_CRC            : CRC error detected
 * \return ERR_FRAMING        : Framing error detected
 * \return ERR_PROTO          : Protocol error detected
 * \return ERR_TIMEOUT        : Timeout error
 * \return ERR_NONE           : No error
 *****************************************************************************
 */
ReturnCode rfalNfcvPollerGetBlockSecurityStatus( uint8_t flags, const uint8_t* uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t* rxBuf, uint16_t rxBufLen, uint16_t *rcvLen )
{
    uint8_t            data[(RFAL_NFCV_BLOCKNUM_LEN + RFAL_NFCV_BLOCKNUM_LEN)];
    uint8_t            dataLen;
    
    dataLen = 0U;
    
    /* Compute Request Data */
    data[dataLen++] = firstBlockNum;                    /* Set first Block Number       */
    data[dataLen++] = numOfBlocks;                      /* Set number of blocks to read */
    
    return rfalNfcvPollerTransceiveReq( RFAL_NFCV_CMD_GET_BLK_SECURITY_STATUS, flags, RFAL_NFCV_PARAM_SKIP, uid, data, dataLen, rxBuf, rxBufLen, rcvLen );
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
