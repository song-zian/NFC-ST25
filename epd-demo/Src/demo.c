#include "demo.h"
#include "utils.h"
#include "rfal_nfc.h"
#include "rfal_st25xv.h"

/* Definition of possible states the demo state machine could have */
#define DEMO_ST_NOTINIT 0         /*!< Demo State:  Not initialized        */
#define DEMO_ST_START_DISCOVERY 1 /*!< Demo State:  Start Discovery        */
#define DEMO_ST_DISCOVERY 2       /*!< Demo State:  Discovery              */

#define DEMO_NFCV_BLOCK_LEN 4                       /*!< NFCV Block len                      */
#define RFAL_NFCV_CMD_GET_BLK_SECURITY_STATUS 0x2CU /*!< Get System Information command                               */
//#define DEMO_NFCV_USE_SELECT_MODE     false /*!< NFCV demonstrate select mode        */
#define DEMO_NFCV_WRITE_TAG true   /*!< NFCV demonstrate Write Single Block */
#define DEMO_NFCV_LOCK_BLOCK false //CL/*!< NFCV demonstrate Lock Single Block */

static rfalNfcDiscoverParam discParam;
static uint8_t state = DEMO_ST_NOTINIT;

extern uint8_t nfcbuf[];
extern uint8_t nfcbuf2[];
uint8_t nfcbuf1[10][125][4] = {0}; //发送10次 140块 每块4个字节；

static void demoNfcv(rfalNfcvListenDevice *nfcvDev);
static void demo2Nfcv(rfalNfcvListenDevice *nfcvDev);
static void demoNotif(rfalNfcState st);
ReturnCode demoTransceiveBlocking(uint8_t *txBuf, uint16_t txBufSize, uint8_t **rxBuf, uint16_t **rcvLen, uint32_t fwt);
ReturnCode rfalNfcvPollerGetBlockSecurityStatus(uint8_t flags, const uint8_t *uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);

/*!
 *****************************************************************************
 * \brief Demo Notification
 *
 *  This function receives the event notifications from RFAL
 *****************************************************************************
 */
static void demoNotif(rfalNfcState st)
{
    uint8_t devCnt;
    rfalNfcDevice *dev;

    if (st == RFAL_NFC_STATE_WAKEUP_MODE)
    {
        printf("Wake Up mode started \r\n");
    }
    else if (st == RFAL_NFC_STATE_POLL_TECHDETECT)
    {
        printf("Wake Up mode terminated. Polling for devices \r\n");
    }
    else if (st == RFAL_NFC_STATE_POLL_SELECT)
    {
        /* Multiple devices were found, activate first of them */
        rfalNfcGetDevicesFound(&dev, &devCnt);
        rfalNfcSelect(0);

        printf("Multiple Tags detected: %d \r\n", devCnt);
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
bool demoIni(void)
{
    ReturnCode err;

    err = rfalNfcInitialize();
    if (err == ERR_NONE)
    {
        discParam.compMode = RFAL_COMPLIANCE_MODE_NFC;
        discParam.devLimit = 1U;

        discParam.notifyCb = demoNotif;
        discParam.totalDuration = 1000U;
        discParam.wakeupEnabled = false;
        discParam.wakeupConfigDefault = true;
        discParam.techs2Find = (RFAL_NFC_POLL_TECH_V);

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
void demoCycle(void)
{
    static rfalNfcDevice *nfcDevice;

    rfalNfcWorker(); /* Run RFAL worker periodically */

    /*******************************************************************************/
    /* Check if USER button is pressed */
    if (platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
    {
        discParam.wakeupEnabled = !discParam.wakeupEnabled; /* enable/disable wakeup */
        state = DEMO_ST_START_DISCOVERY;                    /* restart loop          */
        printf("Toggling Wake Up mode %s\r\n", discParam.wakeupEnabled ? "ON" : "OFF");

        /* Debounce button */
        while (platformGpioIsLow(PLATFORM_USER_BUTTON_PORT, PLATFORM_USER_BUTTON_PIN))
            ;
    }

    switch (state)
    {
    /*******************************************************************************/
    case DEMO_ST_START_DISCOVERY:
        platformLedOff(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);

        rfalNfcDeactivate(false);
        rfalNfcDiscover(&discParam);

        state = DEMO_ST_DISCOVERY;
        break;
    /*******************************************************************************/
    case DEMO_ST_DISCOVERY:

        if (rfalNfcIsDevActivated(rfalNfcGetState()))
        {
            rfalNfcGetActiveDevice(&nfcDevice);

            switch (nfcDevice->type)
            {
            /*******************************************************************************/

            /*******************************************************************************/

            /*******************************************************************************/
            case RFAL_NFC_LISTEN_TYPE_NFCV:
            {
                uint8_t devUID[RFAL_NFCV_UID_LEN];

                ST_MEMCPY(devUID, nfcDevice->nfcid, nfcDevice->nfcidLen); /* Copy the UID into local var */
                REVERSE_BYTES(devUID, RFAL_NFCV_UID_LEN);                 /* Reverse the UID for display purposes */
                printf("ISO15693/NFC-V card found. UID: %s\r\n", hex2Str(devUID, RFAL_NFCV_UID_LEN));

                platformLedOn(PLATFORM_LED_V_PORT, PLATFORM_LED_V_PIN);

                demo2Nfcv(&nfcDevice->dev.nfcv);
            }
            break;

            /*******************************************************************************/
            default:
                break;
            }

            rfalNfcDeactivate(false);
            platformDelay(500);
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
static void demoNfcv(rfalNfcvListenDevice *nfcvDev)
{
    ReturnCode err;
    uint16_t rcvLen;
    uint8_t blockNum = 0x00;
    uint8_t rxBuf[1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN]; /* Flags + Block Data + CRC */
    uint8_t *uid;
#if DEMO_NFCV_WRITE_TAG
    uint8_t wrData[DEMO_NFCV_BLOCK_LEN] = {0x01, 0x01, 0x01, 0x01}; /* Write block example */
#endif                                                              /* DEMO_NFCV_WRITE_TAG */

    uid = nfcvDev->InvRes.UID;

    /*
    * Read block using Read Single Block command
    * with addressed mode (uid != NULL) or selected mode (uid == NULL)
    */
#if DEMO_NFCV_WRITE_TAG /* Writing example */
    err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, sizeof(wrData));
    printf(" Write Block %X: %s Data: %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK", hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
    err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
    printf(" Read Block %X: %s %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK Data:", (err != ERR_NONE) ? "" : hex2Str(&rxBuf[1], DEMO_NFCV_BLOCK_LEN));
#endif /* DEMO_NFCV_WRITE_TAG */
}

static void demo2Nfcv(rfalNfcvListenDevice *nfcvDev)
{
    ReturnCode err;
    uint16_t rcvLen;
    uint8_t blockNum = 0;
    uint8_t rxBuf[1 + DEMO_NFCV_BLOCK_LEN + RFAL_CRC_LEN]; /* Flags + Block Data + CRC */
    uint8_t *uid;
    uint8_t cir = 0; //循环次数
    uint8_t wrData[DEMO_NFCV_BLOCK_LEN] = {0};         /* Write block example */
                                                       /* DEMO_NFCV_WRITE_TAG */
    uid = nfcvDev->InvRes.UID;
		
    for(cir=0;cir<10;cir++)
		{
		  for(blockNum=0;blockNum<125;blockNum++)
		  {
			  for(uint8_t i=0;i<4;i++)
			  {
			    wrData[i] = nfcbuf1[cir][blockNum][i]; 
			  }	
		    err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, sizeof(wrData));
        //printf(" Write Block %X: %s Data: %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK", hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
				
				if(err!=ERR_NONE)//写入失败重新发送
				{
				  err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, wrData, sizeof(wrData));
          //printf(" Write Block %X: %s Data: %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK", hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
				}
        //err = rfalNfcvPollerReadSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, blockNum, rxBuf, sizeof(rxBuf), &rcvLen);
        //printf(" Read Block %X: %s %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK Data:", (err != ERR_NONE) ? "" : hex2Str(&rxBuf[1], DEMO_NFCV_BLOCK_LEN));
		  }
		  
			//先去读取 st25dv是否将数据存储完了 先用delay代替吧
			wrData[0] = 0xaa;  //启动传输
			wrData[1] = cir;   //这是第几次循环
			wrData[2] = 0;
			wrData[3] = 0;
			err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, 125, wrData, sizeof(wrData));
      //printf(" Write Block %X: %s Data: %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK", hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
			if(err!=ERR_NONE)//写入失败重新发送
			{
				err = rfalNfcvPollerWriteSingleBlock(RFAL_NFCV_REQ_FLAG_DEFAULT, uid, 125, wrData, sizeof(wrData));
        //printf(" Write Block %X: %s Data: %s\r\n", blockNum, (err != ERR_NONE) ? "FAIL" : "OK", hex2Str(wrData, DEMO_NFCV_BLOCK_LEN));
			}
			HAL_Delay(1000);
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
ReturnCode rfalNfcvPollerGetBlockSecurityStatus(uint8_t flags, const uint8_t *uid, uint8_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
    uint8_t data[(RFAL_NFCV_BLOCKNUM_LEN + RFAL_NFCV_BLOCKNUM_LEN)];
    uint8_t dataLen;

    dataLen = 0U;

    /* Compute Request Data */
    data[dataLen++] = firstBlockNum; /* Set first Block Number       */
    data[dataLen++] = numOfBlocks;   /* Set number of blocks to read */

    return rfalNfcvPollerTransceiveReq(RFAL_NFCV_CMD_GET_BLK_SECURITY_STATUS, flags, RFAL_NFCV_PARAM_SKIP, uid, data, dataLen, rxBuf, rxBufLen, rcvLen);
}



void nfcBufInit()
{
  int num = 0;	
	for(uint8_t i=0;i<10;i++)
	{
	  for(uint8_t j=0;j<125;j++)
		{
		  for(uint8_t k=0;k<4;k++)
			{
			  nfcbuf1[i][j][k] = nfcbuf[num++]; 
				printf("%d %d %d\r\n",nfcbuf1[i][j][k],nfcbuf[num-1],num-1);
			}
		}
	}
	num = 0;
}

void nfcBuf2Init()
{
  int num = 0;	
	for(uint8_t i=0;i<10;i++)
	{
	  for(uint8_t j=0;j<125;j++)
		{
		  for(uint8_t k=0;k<4;k++)
			{
			  nfcbuf1[i][j][k] = nfcbuf2[num++]; 
				printf("%d %d %d\r\n",nfcbuf1[i][j][k],nfcbuf2[num-1],num-1);
			}
		}
	}
	num = 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
