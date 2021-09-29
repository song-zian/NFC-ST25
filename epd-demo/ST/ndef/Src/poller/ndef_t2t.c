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
 *  This module provides an interface to perform as a NFC Reader/Writer
 *  to handle a Type 2 Tag T2T
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
 #include "ndef_poller.h"
 #include "ndef_t2t.h"
 #include "utils.h"

/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef RFAL_FEATURE_T2T
    #error " RFAL: Module configuration missing. Please enable/disable T2T module by setting: RFAL_FEATURE_T2T "
#endif

#if RFAL_FEATURE_T2T


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T2T_BLOCK_SIZE            4U         /*!< block size                                        */
#define NDEF_T2T_MAX_SECTOR          255U         /*!< Max Number of Sector in Sector Select Command     */ /* 00h -- FEh: 255 sectors */
#define NDEF_T2T_BLOCKS_PER_SECTOR   256U         /*!< Number of Block per Sector                        */
#define NDEF_T2T_BYTES_PER_SECTOR (NDEF_T2T_BLOCKS_PER_SECTOR * NDEF_T2T_BLOCK_SIZE) /*!< Number of Bytes per Sector                        */
#define NDEF_T2T_MAX_OFFSET       (NDEF_T2T_BYTES_PER_SECTOR  * NDEF_T2T_MAX_SECTOR) /*!< Maximum offset allowed                            */
#define NDEF_T2T_3_BYTES_TLV_LEN    0xFFU         /* FFh indicates the use of 3 bytes got the L field    */
#define NDEF_T2T_STATIC_MEM_SIZE      48U         /* Static memory size                                  */

#define NDEF_T2T_CC_OFFSET            12U         /*!< CC offset                                         */
#define NDEF_T2T_CC_LEN                4U         /*!< CC length                                         */
#define NDEF_T2T_AREA_OFFSET          16U         /*!< T2T Area starts at block #4                       */

#define NDEF_T2T_MAGIC              0xE1U         /*!< CC Magic Number                                   */
#define NDEF_T2T_CC_0                  0U         /*!< CC_0: Magic Number                                */
#define NDEF_T2T_CC_1                  1U         /*!< CC_1: Version                                     */
#define NDEF_T2T_CC_2                  2U         /*!< CC_2: Size                                        */
#define NDEF_T2T_CC_3                  3U         /*!< CC_3: Access conditions                           */

#define NDEF_T2T_VERSION_1_0        0x10U         /*!< Version 1.0                                       */

#define NDEF_T2T_SIZE_DIVIDER          8U         /*!<  T2T_area size is measured in bytes is equal to 8 * Size */

#define NDEF_T2T_TLV_NULL           0x00U         /*!< Null TLV                                          */
#define NDEF_T2T_TLV_LOCK_CTRL      0x01U         /*!< Lock Control TLV                                  */
#define NDEF_T2T_TLV_MEMORY_CTRL    0x02U         /*!< Memory Control TLV                                */
#define NDEF_T2T_TLV_NDEF_MESSAGE   0x03U         /*!< NDEF Message TLV                                  */
#define NDEF_T2T_TLV_PROPRIETRARY   0xFDU         /*!< Proprietary TLV                                   */
#define NDEF_T2T_TLV_TERMINATOR     0xFEU         /*!< Terminator TLV                                    */

#define NDEF_T2T_TLV_L_3_BYTES_LEN     3U         /*!< TLV L Length: 3 bytes                             */
#define NDEF_T2T_TLV_L_1_BYTES_LEN     1U         /*!< TLV L Length: 1 bytes                             */
#define NDEF_T2T_TLV_T_LEN             1U         /*!< TLV T Length: 1 bytes                             */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefT2TisT2TDevice(device) ((((device)->type == RFAL_NFC_LISTEN_TYPE_NFCA) && ((device)->dev.nfca.type == RFAL_NFCA_T2T)))
#define ndefT2TInvalidateCache(ctx) { (ctx)->subCtx.t2t.cacheAddr = 0xFFFFFFFFU; }


#define ndefT2TIsReadOnlyAccessGranted(ctx)  (((ctx)->cc.t2t.readAccess == 0x0U) && ((ctx)->cc.t2t.writeAccess == 0xFU))
#define ndefT2TIsReadWriteAccessGranted(ctx) (((ctx)->cc.t2t.readAccess == 0x0U) && ((ctx)->cc.t2t.writeAccess == 0x0U))

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
static ReturnCode ndefT2TPollerReadBlock(ndefContext *ctx, uint16_t blockAddr, uint8_t *buf);

#if NDEF_FEATURE_ALL
static ReturnCode ndefT2TPollerWriteBlock(ndefContext *ctx, uint16_t blockAddr, const uint8_t *buf);
#endif /* NDEF_FEATURE_ALL */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
static ReturnCode ndefT2TPollerReadBlock(ndefContext *ctx, uint16_t blockAddr, uint8_t *buf)
{
    ReturnCode           ret;
    uint8_t              secNo;
    uint8_t              blNo;
    uint16_t             rcvdLen;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    secNo = (uint8_t)(blockAddr >> 8U);
    blNo  = (uint8_t)blockAddr;

    if( secNo != ctx->subCtx.t2t.currentSecNo )
    {
        ret = rfalT2TPollerSectorSelect(secNo);
        if( ret != ERR_NONE )
        {
            return ret;
        }
        ctx->subCtx.t2t.currentSecNo = secNo;
    }

    ret = rfalT2TPollerRead(blNo, buf, NDEF_T2T_READ_RESP_SIZE, &rcvdLen);

    if( (ret == ERR_NONE) && (rcvdLen != NDEF_T2T_READ_RESP_SIZE) )
    {
        return ERR_PROTO;
    }

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen)
{
    ReturnCode           ret;
    uint8_t              le;
    uint32_t             lvOffset = offset;
    uint32_t             lvLen    = len;
    uint8_t *            lvBuf    = buf;
    uint16_t             blockAddr;
    uint8_t              byteNo;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || (lvLen == 0U) || (offset > NDEF_T2T_MAX_OFFSET) )
    {
        return ERR_PARAM;
    }

    if( (offset >= ctx->subCtx.t2t.cacheAddr) && (offset < (ctx->subCtx.t2t.cacheAddr + NDEF_T2T_READ_RESP_SIZE)) && ((offset + len) < (ctx->subCtx.t2t.cacheAddr + NDEF_T2T_READ_RESP_SIZE)) )
    {
        /* data in cache buffer */
        (void)ST_MEMCPY(lvBuf, &ctx->subCtx.t2t.cacheBuf[offset - ctx->subCtx.t2t.cacheAddr], len);
    }
    else
    {
        do {
            blockAddr = (uint16_t)(lvOffset / NDEF_T2T_BLOCK_SIZE);
            byteNo    =  (uint8_t)(lvOffset % NDEF_T2T_BLOCK_SIZE);
            le = (lvLen < NDEF_T2T_READ_RESP_SIZE) ? (uint8_t)lvLen : (uint8_t)NDEF_T2T_READ_RESP_SIZE;

            if( (byteNo != 0U ) || (lvLen < NDEF_T2T_READ_RESP_SIZE) )
            {
                ret = ndefT2TPollerReadBlock(ctx, blockAddr, ctx->subCtx.t2t.cacheBuf);
                if( ret != ERR_NONE )
                {
                    ndefT2TInvalidateCache(ctx);
                    return ret;
                }
                ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
                if( (NDEF_T2T_READ_RESP_SIZE - byteNo) < le )
                {
                    le = NDEF_T2T_READ_RESP_SIZE - byteNo;
                }
                if( le > 0U)
                {
                    (void)ST_MEMCPY(lvBuf, &ctx->subCtx.t2t.cacheBuf[byteNo], le);
                }
            }
            else
            {
                ret = ndefT2TPollerReadBlock(ctx, blockAddr, lvBuf);
                if( ret != ERR_NONE )
                {
                    return ret;
                }
                if( lvLen == le )
                {
                    /* cache the last read block */
                    (void)ST_MEMCPY(&ctx->subCtx.t2t.cacheBuf[0], lvBuf, NDEF_T2T_READ_RESP_SIZE);
                    ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
                }
            }
            lvBuf     = &lvBuf[le];
            lvOffset += le;
            lvLen    -= le;

        } while( lvLen != 0U );
    }

    if( rcvdLen != NULL )
    {
        *rcvdLen = len;
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev)
{
    if( (ctx == NULL) || (dev == NULL) || !ndefT2TisT2TDevice(dev) )
    {
        return ERR_PARAM;
    }

    (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

    ctx->state                   = NDEF_STATE_INVALID;
    ctx->subCtx.t2t.currentSecNo = 0U;
    ndefT2TInvalidateCache(ctx);

   return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerNdefDetect(ndefContext *ctx, ndefInfo *info)
{
    ReturnCode           ret;
    uint8_t              data[2];
    uint32_t             offset;
    uint16_t             lenTLV;
    uint8_t              typeTLV;

    if( info != NULL )
    {
        info->state                = NDEF_STATE_INVALID;
        info->majorVersion         = 0U;
        info->minorVersion         = 0U;
        info->areaLen              = 0U;
        info->areaAvalableSpaceLen = 0U;
        info->messageLen           = 0U;
    }

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    ctx->state = NDEF_STATE_INVALID;

    /* Read CC TS T2T v1.0 7.5.1.1 */
    ret = ndefT2TPollerReadBytes(ctx, NDEF_T2T_CC_OFFSET, NDEF_T2T_CC_LEN, ctx->ccBuf, NULL);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        return ret;
    }
    ctx->cc.t2t.magicNumber  = ctx->ccBuf[NDEF_T2T_CC_0];
    ctx->cc.t2t.majorVersion = ndefMajorVersion(ctx->ccBuf[NDEF_T2T_CC_1]);
    ctx->cc.t2t.minorVersion = ndefMinorVersion(ctx->ccBuf[NDEF_T2T_CC_1]);
    ctx->cc.t2t.size         = ctx->ccBuf[NDEF_T2T_CC_2];
    ctx->cc.t2t.readAccess   = (uint8_t)(ctx->ccBuf[NDEF_T2T_CC_3] >> 4U);
    ctx->cc.t2t.writeAccess  = (uint8_t)(ctx->ccBuf[NDEF_T2T_CC_3] & 0xFU);
    ctx->areaLen = (uint32_t)ctx->cc.t2t.size * NDEF_T2T_SIZE_DIVIDER;
    /* Check version number TS T2T v1.0 7.5.1.2 */
    if( (ctx->cc.t2t.magicNumber != NDEF_T2T_MAGIC) || (ctx->cc.t2t.majorVersion > ndefMajorVersion(NDEF_T2T_VERSION_1_0)) )
    {
        /* Conclude procedure TS T2T v1.0 7.5.1.2 */
        return ERR_REQUEST;
    }
    /* Search for NDEF message TLV TS T2T v1.0 7.5.1.3 */
    offset = NDEF_T2T_AREA_OFFSET;
    while ( (offset < (NDEF_T2T_AREA_OFFSET + ctx->areaLen)) )
    {
        ret = ndefT2TPollerReadBytes(ctx, offset, 1, data, NULL);
        if( ret != ERR_NONE )
        {
            /* Conclude procedure */
            return ret;
        }
        typeTLV = data[0];
        if( typeTLV == NDEF_T2T_TLV_NDEF_MESSAGE )
        {
            ctx->subCtx.t2t.offsetNdefTLV = offset;
        }
        offset++;
        if( typeTLV == NDEF_T2T_TLV_TERMINATOR )
        {
            break;
        }
        if( typeTLV == NDEF_T2T_TLV_NULL )
        {
            continue;
        }
        /* read TLV Len */
        ret = ndefT2TPollerReadBytes(ctx, offset, 1, data, NULL);
        if( ret != ERR_NONE )
        {
            /* Conclude procedure */
            return ret;
        }
        offset++;
        lenTLV = data[0];
        if( lenTLV == NDEF_T2T_3_BYTES_TLV_LEN )
        {
            ret = ndefT2TPollerReadBytes(ctx, offset, 2, data, NULL);
            if( ret != ERR_NONE )
            {
                /* Conclude procedure */
                return ret;
            }
            offset += 2U;
            lenTLV = GETU16(&data[0]);
        }

        if( (typeTLV == NDEF_T2T_TLV_LOCK_CTRL) || (typeTLV == NDEF_T2T_TLV_MEMORY_CTRL) )
        {
            /* No support of Lock control or Memory control in this version */
            return ERR_REQUEST;
        }
        /* NDEF message present TLV TS T2T v1.0 7.5.1.4 */
        if( typeTLV == NDEF_T2T_TLV_NDEF_MESSAGE )
        {
            /* Read length TS T2T v1.0 7.5.1.5 */
            ctx->messageLen    = lenTLV;
            ctx->messageOffset = offset;
            if( ctx->messageLen == 0U )
            {
                if( !(ndefT2TIsReadWriteAccessGranted(ctx)) )
                {
                    /* Conclude procedure  */
                    return ERR_REQUEST;
                }
                 /* Empty message found TS T2T v1.0 7.5.1.6 & TS T2T v1.0 7.4.2.1 */
                ctx->state = NDEF_STATE_INITIALIZED;
            }
            else
            {
                if( (ndefT2TIsReadWriteAccessGranted(ctx)) )
                {
                    /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.3.1 */
                    ctx->state = NDEF_STATE_READWRITE;
                }
                else
                {
                    if( !(ndefT2TIsReadOnlyAccessGranted(ctx)) )
                    {
                        /* Conclude procedure  */
                        return ERR_REQUEST;
                    }
                     /* Empty message found TS T2T v1.0 7.5.1.7 & TS T2T v1.0 7.4.4.1 */
                    ctx->state = NDEF_STATE_READONLY;
                }
            }
            if( info != NULL )
            {
                info->state                = ctx->state;
                info->majorVersion         = ndefMajorVersion(ctx->cc.t4t.vNo);
                info->minorVersion         = ndefMinorVersion(ctx->cc.t4t.vNo);
                info->areaLen              = ctx->areaLen;
                info->areaAvalableSpaceLen = ctx->areaLen - ctx->messageOffset;
                info->messageLen           = ctx->messageLen;
            }
            return ERR_NONE;
        }
        offset += lenTLV;
    }
    return ERR_REQUEST;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen)
{
    ReturnCode           ret;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    /* TS T2T v1.0 7.5.2.1: T2T NDEF Detect should have been called before NDEF read procedure */
    /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF read procedure*/

    /* TS T2T v1.0 7.5.2.3: check presence of NDEF message */
    if ( ctx->state <= NDEF_STATE_INITIALIZED )
    {
        /* Conclude procedure TS T4T v1.0 7.2.2.2 */
        return ERR_WRONG_STATE;
    }

    if( ctx->messageLen > bufLen )
    {
        return ERR_NOMEM;
    }

    /* Current implementation does not support Rsvd_area */
    ret = ndefT2TPollerReadBytes( ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen );
    if( ret != ERR_NONE )
    {
        ctx->state = NDEF_STATE_INVALID;
    }
    return ret;
}

#if NDEF_FEATURE_ALL

/*******************************************************************************/
static ReturnCode ndefT2TPollerWriteBlock(ndefContext *ctx, uint16_t blockAddr, const uint8_t *buf)
{
    ReturnCode           ret;
    uint8_t              secNo;
    uint8_t              blNo;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    secNo = (uint8_t)(blockAddr >> 8U);
    blNo  = (uint8_t)blockAddr;

    if( secNo != ctx->subCtx.t2t.currentSecNo )
    {
        ret = rfalT2TPollerSectorSelect(secNo);
        if( ret != ERR_NONE )
        {
            return ret;
        }
        ctx->subCtx.t2t.currentSecNo = secNo;
    }

    ret = rfalT2TPollerWrite(blNo, buf);

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len)
{
    ReturnCode           ret;
    uint32_t             lvOffset = offset;
    uint32_t             lvLen    = len;
    const uint8_t *      lvBuf    = buf;
    uint16_t             blockAddr;
    uint8_t              byteNo;
    uint8_t              le;
    uint8_t              tempBuf[NDEF_T2T_READ_RESP_SIZE];

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || (lvLen == 0U) )
    {
        return ERR_PARAM;
    }

    ndefT2TInvalidateCache(ctx);

    do
    {
        blockAddr = (uint16_t)(lvOffset / NDEF_T2T_BLOCK_SIZE);
        byteNo    =  (uint8_t)(lvOffset % NDEF_T2T_BLOCK_SIZE);
        le = (lvLen < NDEF_T2T_BLOCK_SIZE) ? (uint8_t)lvLen : (uint8_t)NDEF_T2T_BLOCK_SIZE;
        if( (byteNo != 0U ) || (lvLen < NDEF_T2T_BLOCK_SIZE) )
        {
            ret = ndefT2TPollerReadBlock(ctx, blockAddr, tempBuf);
            if( ret != ERR_NONE )
            {
                return ret;
            }
            if( (NDEF_T2T_BLOCK_SIZE - byteNo) < le )
            {
                le = NDEF_T2T_BLOCK_SIZE - byteNo;
            }
            if( le > 0U )
            {
                (void)ST_MEMCPY(&tempBuf[byteNo], lvBuf, le);
            }
            ret = ndefT2TPollerWriteBlock(ctx, blockAddr, tempBuf);
            if( ret != ERR_NONE )
            {
                return ret;
            }
        }
        else
        {
            ret = ndefT2TPollerWriteBlock(ctx, blockAddr, lvBuf);
            if( ret != ERR_NONE )
            {
                return ret;
            }
        }
        lvBuf     = &lvBuf[le];
        lvOffset += le;
        lvLen    -= le;

    } while( lvLen != 0U );

    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen)
{
    ReturnCode           ret;
    uint8_t              buf[NDEF_T2T_BLOCK_SIZE];
    uint8_t              dataIt;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        return ERR_WRONG_STATE;
    }
    dataIt = 0U;
    buf[dataIt] = NDEF_T2T_TLV_NDEF_MESSAGE;
    dataIt++;
    if( rawMessageLen <= NDEF_SHORT_VFIELD_MAX_LEN )
    {
        buf[dataIt] = (uint8_t) rawMessageLen;
        dataIt++;
    }
    else
    {
        buf[dataIt] = (uint8_t) (rawMessageLen >> 8U);
        dataIt++;
        buf[dataIt] = (uint8_t) rawMessageLen;
        dataIt++;
    }
    if( rawMessageLen == 0U )
    {
        buf[dataIt] = NDEF_T2T_TLV_TERMINATOR;
        dataIt++;
    }

    ret = ndefT2TPollerWriteBytes(ctx, ctx->subCtx.t2t.offsetNdefTLV, buf, dataIt);
    if( (ret != ERR_NONE) && (rawMessageLen != 0U) && ((ctx->messageOffset + rawMessageLen) < ctx->areaLen) )
    {
        /* Write Terminator TLV */
        dataIt = 0U;
        buf[dataIt] = NDEF_T2T_TLV_TERMINATOR;
        dataIt++;
        (void)ndefT2TPollerWriteBytes(ctx, ctx->messageOffset + rawMessageLen, buf, dataIt );
    }

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen)
{
    ReturnCode ret;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) || ((buf == NULL) && (bufLen != 0U)) )
    {
        return ERR_PARAM;
    }

    /* TS T2T v1.0 7.5.3.1/2: T4T NDEF Detect should have been called before NDEF write procedure */
    /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF Write procedure*/

    /* TS T2T v1.0 7.5.3.3: check write access condition */
    if ( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        /* Conclude procedure */
        return ERR_WRONG_STATE;
    }

    /* TS T2T v1.0 7.5.3.3: verify available space */
    ret = ndefT2TPollerCheckAvailableSpace(ctx, bufLen);
    if( ret != ERR_NONE )
    {
        /* Conclude procedures */
        return ERR_PARAM;
    }

    /* TS T2T v1.0 7.5.3.4: reset L_Field to 0                */
    /* and update ctx->messageOffset according to L-field len */
    ret = ndefT2TPollerBeginWriteMessage(ctx, bufLen);
    if( ret != ERR_NONE )
    {
        ctx->state = NDEF_STATE_INVALID;
        /* Conclude procedure */
        return ret;
    }

    if( bufLen != 0U )
    {
       /* TS T2T v1.0 7.5.3.5: write new NDEF message */
        ret = ndefT2TPollerWriteBytes(ctx, ctx->messageOffset, buf, bufLen);
        if  (ret != ERR_NONE)
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return ret;
        }

        /* TS T2T v1.0 7.5.3.6 & 7.5.3.7: update L_Field and write Terminator TLV */
        ret = ndefT2TPollerEndWriteMessage(ctx, bufLen);
        if( ret != ERR_NONE )
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return ret;
        }
    }

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
    ReturnCode           ret;
    uint8_t              dataIt;
    static const uint8_t emptyNdef[] = {NDEF_T2T_TLV_NDEF_MESSAGE, 0x00U, NDEF_T2T_TLV_TERMINATOR, 0x00U};

    NO_WARNING(options);

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    /*
     * Read CC area
     */
    ret = ndefT2TPollerReadBytes(ctx, NDEF_T2T_CC_OFFSET, NDEF_T2T_CC_LEN, ctx->ccBuf, NULL);
    if( ret != ERR_NONE )
    {
        return ret;
    }

    ndefT2TInvalidateCache(ctx);

    /*
     * Write CC only in case of virgin CC area
     */
    if( (ctx->ccBuf[NDEF_T2T_CC_0] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_1] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_2] == 0U) && (ctx->ccBuf[NDEF_T2T_CC_3] == 0U) )
    {
        dataIt = 0U;
        if( cc == NULL )
        {
            /* Use default values if no cc provided */
            ctx->ccBuf[dataIt] = NDEF_T2T_MAGIC;
            dataIt++;
            ctx->ccBuf[dataIt] = NDEF_T2T_VERSION_1_0;
            dataIt++;
            ctx->ccBuf[dataIt] = NDEF_T2T_STATIC_MEM_SIZE / NDEF_T2T_SIZE_DIVIDER;
            dataIt++;
            ctx->ccBuf[dataIt] = 0x00U;
            dataIt++;
        }
        else
        {
            ctx->ccBuf[dataIt] = cc->t2t.magicNumber;
            dataIt++;
            ctx->ccBuf[dataIt] = (uint8_t)(cc->t2t.majorVersion << 4U) | cc->t2t.minorVersion;
            dataIt++;
            ctx->ccBuf[dataIt] = cc->t2t.size;
            dataIt++;
            ctx->ccBuf[dataIt] = (uint8_t)(cc->t2t.readAccess << 4U) | cc->t2t.writeAccess;
            dataIt++;
        }
        ret = ndefT2TPollerWriteBlock(ctx, NDEF_T2T_CC_OFFSET/NDEF_T2T_BLOCK_SIZE, ctx->ccBuf);
        if( ret != ERR_NONE )
        {
            return ret;
        }
    }

    /*
     * Write NDEF place holder
     */
    ret = ndefT2TPollerWriteBlock(ctx, NDEF_T2T_AREA_OFFSET/NDEF_T2T_BLOCK_SIZE, emptyNdef);

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerCheckPresence(ndefContext *ctx)
{
    ReturnCode           ret;
    uint16_t             blockAddr;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    blockAddr = 0U;
    ret = ndefT2TPollerReadBlock(ctx, blockAddr, ctx->subCtx.t2t.cacheBuf);
    if( ret != ERR_NONE )
    {
        ndefT2TInvalidateCache(ctx);
        return ret;
    }
    ctx->subCtx.t2t.cacheAddr = (uint32_t)blockAddr * NDEF_T2T_BLOCK_SIZE;
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
    uint32_t             lLen;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if ( ctx->state == NDEF_STATE_INVALID )
    {
        return ERR_WRONG_STATE;
    }

    lLen = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T2T_TLV_L_3_BYTES_LEN : NDEF_T2T_TLV_L_1_BYTES_LEN;

    if( (messageLen + ctx->subCtx.t2t.offsetNdefTLV + NDEF_T2T_TLV_T_LEN + lLen) > (ctx->areaLen + NDEF_T2T_AREA_OFFSET) )
    {
        return ERR_NOMEM;
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode           ret;
    uint32_t             lLen;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T2T v1.0 7.5.3.4: reset L_Field to 0 */
    ret = ndefT2TPollerWriteRawMessageLen(ctx, 0U);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }

    lLen = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T2T_TLV_L_3_BYTES_LEN : NDEF_T2T_TLV_L_1_BYTES_LEN;
    ctx->messageOffset  = ctx->subCtx.t2t.offsetNdefTLV;
    ctx->messageOffset += NDEF_T2T_TLV_T_LEN; /* T Len */
    ctx->messageOffset += lLen;               /* L Len */

    ctx->state = NDEF_STATE_INITIALIZED;

    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT2TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode           ret;

    if( (ctx == NULL) || !ndefT2TisT2TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->state != NDEF_STATE_INITIALIZED )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T2T v1.0 7.5.3.6 & 7.5.3.7: update L_Field and write Terminator TLV */
    ret = ndefT2TPollerWriteRawMessageLen(ctx, messageLen);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }
    ctx->messageLen = messageLen;
    ctx->state = (ctx->messageLen == 0U) ? NDEF_STATE_INITIALIZED : NDEF_STATE_READWRITE;
    return ERR_NONE;
}

#endif /* NDEF_FEATURE_ALL */

#endif /* RFAL_FEATURE_T2T */
