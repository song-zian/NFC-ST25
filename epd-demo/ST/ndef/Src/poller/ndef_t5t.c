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
 *  \brief Provides NDEF methods and definitions to access NFC-V Forum T5T
 *
 *  This module provides an interface to perform as a NFC-V Reader/Writer
 *  to handle a Type 5 Tag T5T
 *
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
 #include "ndef_poller.h"
 #include "ndef_t5t.h"
 #include "utils.h"

 /*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef RFAL_FEATURE_NFCV
    #error " RFAL: Module configuration missing. Please enable/disable T5T support by setting: RFAL_FEATURE_NFCV "
#endif

#if RFAL_FEATURE_NFCV

/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_T5T_UID_MANUFACTURER_ID_POS       6U    /*!< Manufacturer ID Offset in UID buffer (reverse)    */
#define NDEF_T5T_MANUFACTURER_ID_ST         0x02U    /*!< Manufacturer ID for ST                            */

#define NDEF_T5T_SYSINFO_MAX_LEN             22U     /*!< Max length for (Extended) Get System Info response                */

#define NDEF_T5T_MLEN_DIVIDER                  8U    /*!<  T5T_area size is measured in bytes is equal to 8 * MLEN          */

#define NDEF_T5T_TLV_L_3_BYTES_LEN             3U    /*!< TLV L Length: 3 bytes                             */
#define NDEF_T5T_TLV_L_1_BYTES_LEN             1U    /*!< TLV L Length: 1 bytes                             */
#define NDEF_T5T_TLV_T_LEN                     1U    /*!< TLV T Length: 1 bytes                             */

#define NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR       256U    /*!< Max number of blocks for 1 byte addressing        */
#define NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING    256U    /*!< MLEN max value for 1 byte encoding                */

#define NDEF_T5T_TL_MAX_SIZE  (NDEF_T5T_TLV_T_LEN \
                       + NDEF_T5T_TLV_L_3_BYTES_LEN) /*!< Max TL size                                       */

#define NDEF_T5T_TLV_NDEF                   0x03U    /*!< TLV flag NDEF value                               */
#define NDEF_T5T_TLV_PROPRIETARY            0xFDU    /*!< TLV flag PROPRIETARY value                        */
#define NDEF_T5T_TLV_TERMINATOR             0xFEU    /*!< TLV flag TERMINATOR value                         */
#define NDEF_T5T_TLV_RFU                    0x00U    /*!< TLV flag RFU value                                */

/*
 *****************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefT5TisT5TDevice(device) ((device)->type == RFAL_NFC_LISTEN_TYPE_NFCV)

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

static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended);

#if NDEF_FEATURE_ALL
static ReturnCode ndefT5TWriteCC(ndefContext *ctx);
static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t* wrData);
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen);
#endif /* NDEF_FEATURE_ALL */

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */

/*******************************************************************************/
ReturnCode ndefT5TPollerReadBytes(ndefContext * ctx, uint32_t offset, uint32_t len, uint8_t* buf, uint32_t * rcvdLen )
{
    uint8_t         lastVal;
    uint8_t         status;
    uint16_t        res;
    uint16_t        nbRead;
    uint16_t        blockLen;
    uint16_t        startBlock;
    uint16_t        startAddr;
    ReturnCode      result     = ERR_PARAM;
    uint32_t        currentLen = len;
    uint32_t        lvRcvLen   = 0U;

    if ( ( ctx != NULL) && (ctx->subCtx.t5t.blockLen > 0U) && (buf != NULL) && (len > 0U) )
    {
        blockLen   = (uint16_t )ctx->subCtx.t5t.blockLen;
        if( blockLen == 0U )
        {
            return ERR_SYSTEM;
        }
        startBlock = (uint16_t) (offset / blockLen);
        startAddr  = (uint16_t) (startBlock * blockLen);

        res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, blockLen + 3U, &nbRead);
        if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U) )
        {
            nbRead = (uint16_t) (nbRead  + startAddr - (uint16_t)offset - 1U );
            if ((uint32_t) nbRead > currentLen)
            {
                nbRead = (uint16_t) currentLen;
            }
            if (nbRead > 0U)
            {
                (void)ST_MEMCPY(buf, &ctx->subCtx.t5t.txrxBuf[1U - startAddr + (uint16_t)offset], (uint32_t)nbRead);
            }
            lvRcvLen   += (uint32_t) nbRead;
            currentLen -= (uint32_t) nbRead;
            while (currentLen >= ((uint32_t)blockLen + 2U) )
            {
                startBlock++;
                lastVal = buf[lvRcvLen - 1U];
                res = ndefT5TPollerReadSingleBlock(ctx, startBlock, &buf[lvRcvLen - 1U], blockLen + 3U, &nbRead);
                status  = buf[lvRcvLen - 1U]; /* Keep status */
                buf[lvRcvLen - 1U] = lastVal; /* Restore previous value */
                if ( (res == ERR_NONE) && (nbRead > 0U) && (status == 0U))
                {
                    lvRcvLen   += blockLen;
                    currentLen -= blockLen;
                }
                else
                {
                    break;
                }
            }
            while (currentLen > 0U)
            {
                startBlock++;
                res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, blockLen + 3U, &nbRead);
                if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U))
                {
                    -- nbRead; /* remove status char */
                    if (nbRead > currentLen)
                    {
                        nbRead = (uint16_t)currentLen;
                    }
                    if (nbRead > 0U)
                    {
                        (void)ST_MEMCPY(&buf[lvRcvLen], & ctx->subCtx.t5t.txrxBuf[1U], nbRead);
                    }
                    lvRcvLen   += nbRead;
                    currentLen -= nbRead;
                }
                else
                {
                    break;
                }
            }
        }
    }
    if (currentLen == 0U)
    {
        result = ERR_NONE;
    }
    if( rcvdLen != NULL )
    {
        * rcvdLen = lvRcvLen;
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev)
{
    ReturnCode    result;
    uint16_t      rcvLen;

    if( (ctx == NULL) || (dev == NULL) || !ndefT5TisT5TDevice(dev) )
    {
        return ERR_PARAM;
    }

    (void)ST_MEMCPY(&ctx->device, dev, sizeof(ctx->device));

    /* Reset info about the card */
    ctx->state                    = NDEF_STATE_INVALID;
    ctx->messageOffset            = 0U;
    ctx->messageLen               = 0U;
    ctx->subCtx.t5t.blockLen      = 0U;
    ctx->subCtx.t5t.pAddressedUid = ctx->device.dev.nfcv.InvRes.UID; /* By default work in addressed mode */
    ctx->subCtx.t5t.TlvNDEFOffset = 0U; /* Offset for TLV */

    ctx->subCtx.t5t.legacySTHighDensity = false;
    result = ndefT5TPollerReadSingleBlock( ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen );
    if( (result != ERR_NONE) && (ctx->device.dev.nfcv.InvRes.UID[NDEF_T5T_UID_MANUFACTURER_ID_POS] == NDEF_T5T_MANUFACTURER_ID_ST) )
    {
        /* Try High Density Legacy mode */
        ctx->subCtx.t5t.legacySTHighDensity = true;
        result = ndefT5TPollerReadSingleBlock( ctx, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen );
        if( result != ERR_NONE )
        {
            return result;
        }
    }

    if( (rcvLen > 1U) && (ctx->subCtx.t5t.txrxBuf[0U] == (uint8_t) 0U) )
    {
        ctx->subCtx.t5t.blockLen = (uint8_t) (rcvLen - 1U);
    }
    else
    {
        return ERR_PROTO;
    }

    if (rfalNfcvPollerSelect( (uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->device.dev.nfcv.InvRes.UID)  == ERR_NONE)
    {
        ctx->subCtx.t5t.pAddressedUid = NULL; /* Switch to selected mode */
    }

    ctx->subCtx.t5t.sysInfoSupported = false;

    if( !ctx->subCtx.t5t.legacySTHighDensity)
    {
        /* Extended Get System Info */
        if( ndefT5TGetSystemInformation(ctx, true) == ERR_NONE )
        {
            ctx->subCtx.t5t.sysInfoSupported = true;
        }
    }
    if( !ctx->subCtx.t5t.sysInfoSupported )
    {
        /* Get System Info */
        if( ndefT5TGetSystemInformation(ctx, false) == ERR_NONE )
        {
            ctx->subCtx.t5t.sysInfoSupported = true;
        }
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerNdefDetect(ndefContext * ctx, ndefInfo *info)
{
    ReturnCode result;
    uint8_t    tmpBuf[NDEF_T5T_TL_MAX_SIZE];
    ReturnCode returnCode = ERR_REQUEST; /* Default return code */
    uint16_t   offset;
    uint16_t   length;
    uint32_t   TlvOffset;
    bool       bExit;
    uint32_t   rcvLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    ctx->state                           = NDEF_STATE_INVALID;
    ctx->cc.t5t.ccLen                    = 0U;
    ctx->cc.t5t.memoryLen                = 0U;
    ctx->messageLen                      = 0U;
    ctx->messageOffset                   = 0U;

    if( info != NULL )
    {
        info->state                = NDEF_STATE_INVALID;
        info->majorVersion         = 0U;
        info->minorVersion         = 0U;
        info->areaLen              = 0U;
        info->areaAvalableSpaceLen = 0U;
        info->messageLen           = 0U;
    }

    result = ndefT5TPollerReadBytes(ctx, 0U, 8U, ctx->ccBuf, &rcvLen);
    if ( (result == ERR_NONE) && (rcvLen == 8U) && ( (ctx->ccBuf[0] == (uint8_t)0xE1U) || (ctx->ccBuf[0] == (uint8_t)0xE2U) ) )
    {
        ctx->cc.t5t.magicNumber           =  ctx->ccBuf[0U];
        ctx->cc.t5t.majorVersion          = (ctx->ccBuf[1U] >> 6U ) & 0x03U;
        ctx->cc.t5t.minorVersion          = (ctx->ccBuf[1U] >> 4U ) & 0x03U;
        ctx->cc.t5t.readAccess            = (ctx->ccBuf[1U] >> 2U ) & 0x03U;
        ctx->cc.t5t.writeAccess           = (ctx->ccBuf[1U] >> 0U ) & 0x03U;
        ctx->cc.t5t.memoryLen             =  ctx->ccBuf[2U];
        ctx->cc.t5t.multipleBlockRead     = (((ctx->ccBuf[3U] >> 0U ) & 0x01U) != 0U);
        ctx->cc.t5t.mlenOverflow          = (((ctx->ccBuf[3U] >> 2U ) & 0x01U) != 0U);
        ctx->cc.t5t.lockBlock             = (((ctx->ccBuf[3U] >> 3U ) & 0x01U) != 0U);
        ctx->cc.t5t.specialFrame          = (((ctx->ccBuf[3U] >> 4U ) & 0x01U) != 0U);
        ctx->state                        = NDEF_STATE_INITIALIZED;

        if ( ctx->cc.t5t.memoryLen != 0U)
        {
            ctx->cc.t5t.ccLen             = NDEF_T5T_CC_LEN_4_BYTES;
            if( (ctx->cc.t5t.memoryLen == 0xFFU) && ctx->cc.t5t.mlenOverflow )
            {
                if( (ctx->subCtx.t5t.sysInfoSupported==true) && ( ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
                {
                    ctx->cc.t5t.memoryLen = (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock  * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);
                }
            }
        }
        else
        {
            ctx->cc.t5t.ccLen             = NDEF_T5T_CC_LEN_8_BYTES;
            ctx->cc.t5t.memoryLen         = ((uint16_t)ctx->ccBuf[6U] << 8U) + (uint16_t)ctx->ccBuf[7U];
        }
        if( (ctx->subCtx.t5t.sysInfoSupported==true) &&
            (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags)!= 0U) &&
            (ctx->cc.t5t.memoryLen == (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock  * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER)) &&
            (ctx->cc.t5t.memoryLen > 0U) )
        {
            ctx->cc.t5t.memoryLen--; /* remove CC area from memory len */
        }
        ctx->messageLen     = 0U;
        ctx->messageOffset  = ctx->cc.t5t.ccLen;
        TlvOffset = ctx->cc.t5t.ccLen;
        bExit     = false;
        do
        {
            result = ndefT5TPollerReadBytes(ctx, TlvOffset, NDEF_T5T_TL_MAX_SIZE, tmpBuf, &rcvLen);
            if ( (result != ERR_NONE) || ( rcvLen != NDEF_T5T_TL_MAX_SIZE) )
            {
                break;
            }
            offset = 2U;
            length = tmpBuf[1U];
            if ( length == (NDEF_SHORT_VFIELD_MAX_LEN + 1U) )
            {
                /* Size is encoded in 1 + 2 bytes */
                length = (((uint16_t)tmpBuf[2U]) << 8U) + (uint16_t)tmpBuf[3U];
                offset += 2U;
            }
            if (tmpBuf[0U] == (uint8_t)NDEF_T5T_TLV_NDEF)
            {
                /* NDEF record return it */
                returnCode                    = ERR_NONE;  /* Default */
                ctx->subCtx.t5t.TlvNDEFOffset = TlvOffset; /* Offset for TLV */
                ctx->messageOffset            = TlvOffset + offset;
                ctx->messageLen               = length;
                TlvOffset = 0U;
                if (length == 0U)
                {
                    /* Req 40 7.5.1.6 */
                    if ( (ctx->cc.t5t.readAccess == 0U) && (ctx->cc.t5t.writeAccess == 0U) )
                    {
                        ctx->state = NDEF_STATE_INITIALIZED;
                    }
                    else
                    {
                        ctx->state = NDEF_STATE_INVALID;
                        returnCode = ERR_REQUEST; /* Default */
                    }
                    bExit = true;
                }
                else
                {
                    if (ctx->cc.t5t.readAccess == 0U)
                    {
                        if (ctx->cc.t5t.writeAccess == 0U)
                        {
                            ctx->state = NDEF_STATE_READWRITE;
                        }
                        else
                        {
                            ctx->state = NDEF_STATE_READONLY;
                        }
                    }
                    bExit = true;
                }
            }
            else if (tmpBuf[0U]== (uint8_t) NDEF_T5T_TLV_TERMINATOR)
            {
                /* NDEF end */
                TlvOffset = 0U;
                bExit     = true;
            }
            else if (tmpBuf[0U]== (uint8_t) NDEF_T5T_TLV_PROPRIETARY)
            {   /* proprietary go next, nothing to do */
                TlvOffset +=  (uint32_t)offset + (uint32_t)length;
            }
            else
            {
                /* RFU value */
                TlvOffset = 0U;
                bExit = true;
            }
        } while ( ( TlvOffset > 0U) && (bExit == false) );
    }
    else
    {
        /* No CCFile */
        returnCode = ERR_REQUEST;
        if (result != ERR_NONE)
        {
            returnCode = result;
        }
    }

    /* TS T5T v1.0 4.3.1.17 T5T_area size is measured in bytes is equal to 8 * MLEN */
    ctx->areaLen               = (uint32_t)ctx->cc.t5t.memoryLen * NDEF_T5T_MLEN_DIVIDER;
    if( info != NULL )
    {
        info->state                = ctx->state;
        info->majorVersion         = ctx->cc.t5t.majorVersion;
        info->minorVersion         = ctx->cc.t5t.minorVersion;
        info->areaLen              = ctx->areaLen;
        info->areaAvalableSpaceLen = (uint32_t)ctx->cc.t5t.ccLen + ctx->areaLen - ctx->messageOffset;
        info->messageLen           = ctx->messageLen;
    }
    return returnCode;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen)
{
    ReturnCode result;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    if( ctx->messageLen > bufLen )
    {
        return ERR_NOMEM;
    }

    result = ndefT5TPollerReadBytes( ctx, ctx->messageOffset, ctx->messageLen, buf, rcvdLen );
    return result;
}

#if NDEF_FEATURE_ALL

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t * buf, uint32_t len)
{
    ReturnCode      result = ERR_REQUEST;
    ReturnCode      res;
    uint16_t        nbRead;
    uint16_t        blockLen16;
    uint16_t        startBlock;
    uint16_t        startAddr ;
    const uint8_t * wrbuf      = buf;
    uint32_t        currentLen = len;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (len == 0U) || (ctx->subCtx.t5t.blockLen == 0U))
    {
        return ERR_PARAM;
    }
    blockLen16 = (uint16_t )ctx->subCtx.t5t.blockLen;
    if( blockLen16 == 0U )
    {
        return ERR_SYSTEM;
    }
    startBlock = (uint16_t) (offset     / blockLen16);
    startAddr  = (uint16_t) (startBlock * blockLen16);

    if (startAddr != offset)
    {
        /* Unaligned start offset must read the first block before */
        res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, blockLen16 + 3U, &nbRead);
        if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U) )
        {
            nbRead = (uint16_t) ((uint32_t)nbRead - 1U  + startAddr - offset);
            if (nbRead > (uint32_t) currentLen)
            {
                nbRead = (uint16_t) currentLen;
            }
            if (nbRead > 0U)
            {
                (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[1U - startAddr + (uint16_t)offset], wrbuf, nbRead);
            }
            res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
            if (res != ERR_NONE)
            {
                return res;
            }
        }
        else
        {
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                result = ERR_PARAM;
            }
            return result;
        }
        currentLen -= nbRead;
        wrbuf       = &wrbuf[nbRead];
        startBlock++;
    }
    while (currentLen >= blockLen16)
    {
        res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, wrbuf);
        if (res == ERR_NONE)
        {
            currentLen -= blockLen16;
            wrbuf       = &wrbuf[blockLen16];
            startBlock++;
        }
        else
        {
            result = res;
            break;
        }
    }
    if ( (currentLen != 0U) && (currentLen < blockLen16) )
    {
        /* Unaligned end, must read the first block before */
        res = ndefT5TPollerReadSingleBlock(ctx, startBlock, ctx->subCtx.t5t.txrxBuf, blockLen16 + 3U, &nbRead);
        if ( (res == ERR_NONE) && (ctx->subCtx.t5t.txrxBuf[0U] == 0U) && (nbRead > 0U) )
        {
            if (currentLen > 0U) { (void)ST_MEMCPY(&ctx->subCtx.t5t.txrxBuf[1U], wrbuf, currentLen); }
            res = ndefT5TPollerWriteSingleBlock(ctx, startBlock, &ctx->subCtx.t5t.txrxBuf[1U]);
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                currentLen = 0U;
            }
        }
        else
        {
            if (res != ERR_NONE)
            {
                result = res;
            }
            else
            {
                result = ERR_PARAM;
            }
            return result;
        }
    }
    if (currentLen == 0U)
    {
        result = ERR_NONE;
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen)
{
    uint8_t    TLV[8U];
    ReturnCode result = ERR_PARAM;
    uint8_t     len    = 0U;

    if( (ctx != NULL) && ndefT5TisT5TDevice(&ctx->device))
    {
        if ( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
        {
            result = ERR_WRONG_STATE;
        }
        else
        {
            TLV[len] = NDEF_T5T_TLV_NDEF;
            len++;
            if (rawMessageLen <= NDEF_SHORT_VFIELD_MAX_LEN)
            {
                TLV[len] = (uint8_t) rawMessageLen;
                len++;
            }
            else
            {
                TLV[len] = (uint8_t) (rawMessageLen >> 8U);
                len++;
                TLV[len] = (uint8_t) rawMessageLen;
                len++;
            }
            if (rawMessageLen == 0U)
            {
                TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
                len++;
            }

            result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, TLV, len);
            if ((result == ERR_NONE) && (rawMessageLen != 0U))
            {  /* T5T need specific terminator */
               len = 0U;
               TLV[len] = NDEF_TERMINATOR_TLV_T; /* TLV terminator */
               len++;
               result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset + rawMessageLen, TLV, len );
            }
        }
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerWriteRawMessage(ndefContext *ctx, const uint8_t * buf, uint32_t bufLen)
{
    uint32_t   len = bufLen ;
    ReturnCode result;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) || (buf == NULL) )
    {
        return ERR_PARAM;
    }

    /* TS T5T v1.0 7.5.3.1/2: T5T NDEF Detect should have been called before NDEF write procedure */
    /* Warning: current tag content must not be changed between NDEF Detect procedure and NDEF Write procedure*/

    /* TS T5T v1.0 7.5.3.3: check write access condition */
    if ( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        /* Conclude procedure */
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.3: verify available space */
    result = ndefT5TPollerCheckAvailableSpace(ctx, bufLen);
    if( result != ERR_NONE )
    {
        /* Conclude procedures */
        return ERR_PARAM;
    }
    /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
    /* and update ctx->messageOffset according to L-field len */
    result = ndefT5TPollerBeginWriteMessage(ctx, bufLen);
    if  (result != ERR_NONE)
    {
        ctx->state = NDEF_STATE_INVALID;
        /* Conclude procedure */
        return result;
    }
    if( bufLen != 0U )
    {
        /* TS T5T v1.0 7.5.3.5: write new NDEF message */
        result = ndefT5TPollerWriteBytes(ctx, ctx->messageOffset, buf, len);
        if  (result != ERR_NONE)
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return result;
        }
        /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
        result = ndefT5TPollerEndWriteMessage(ctx, len);
        if  (result != ERR_NONE)
        {
            /* Conclude procedure */
            ctx->state = NDEF_STATE_INVALID;
            return result;
        }
    }
    return result;
}

/*******************************************************************************/
static ReturnCode ndefT5TWriteCC(ndefContext *ctx)
{
    ReturnCode  ret;
    uint8_t*    buf;
    uint8_t     dataIt;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    buf    = ctx->ccBuf;
    dataIt = 0U;
    /* Encode CC */
    buf[dataIt] = ctx->cc.t5t.magicNumber;                                                                /* Byte 0 */
    dataIt++;
    buf[dataIt] = (uint8_t)(((ctx->cc.t5t.majorVersion  & 0x03U) << 6) |                                  /* Byte 1 */
                            ((ctx->cc.t5t.minorVersion  & 0x03U) << 4) |                                  /*        */
                            ((ctx->cc.t5t.readAccess    & 0x03U) << 2) |                                  /*        */
                            ((ctx->cc.t5t.writeAccess   & 0x03U) << 0));                                  /*        */
    dataIt++;
    buf[dataIt] = (ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES) ? 0U : (uint8_t)ctx->cc.t5t.memoryLen;   /* Byte 2 */
    dataIt++;
    buf[dataIt]   = 0U;                                                                                   /* Byte 3 */
    if( ctx->cc.t5t.multipleBlockRead ) { buf[dataIt] |= 0x01U; }                                         /* Byte 3  b0 MBREAD                */
    if( ctx->cc.t5t.mlenOverflow )      { buf[dataIt] |= 0x04U; }                                         /* Byte 3  b2 Android MLEN overflow */
    if( ctx->cc.t5t.lockBlock )         { buf[dataIt] |= 0x08U; }                                         /* Byte 3  b3 Lock Block            */
    if( ctx->cc.t5t.specialFrame )      { buf[dataIt] |= 0x10U; }                                         /* Byte 3  b4 Special Frame         */
    dataIt++;
    if( ctx->cc.t5t.ccLen == NDEF_T5T_CC_LEN_8_BYTES )
    {
        buf[dataIt] = 0U;                                                                                 /* Byte 4 */
        dataIt++;
        buf[dataIt] = 0U;                                                                                 /* Byte 5 */
        dataIt++;
        buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 8);                                              /* Byte 6 */
        dataIt++;
        buf[dataIt] = (uint8_t)(ctx->cc.t5t.memoryLen >> 0);                                              /* Byte 7 */
        dataIt++;
    }

    ret = ndefT5TPollerWriteBytes(ctx, 0U, buf, ctx->cc.t5t.ccLen );
    return ret;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerTagFormat(ndefContext * ctx, const ndefCapabilityContainer *cc, uint32_t options)
{
    uint16_t                 rcvdLen;
    ReturnCode               result;
    static const uint8_t     emptyNDEF[] = { 0x03U, 0x00U, 0xFEU, 0x00U};

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    /* Reset previous potential info about NDEF messages */
    ctx->messageLen               = 0U;
    ctx->messageOffset            = 0U;
    ctx->subCtx.t5t.TlvNDEFOffset = 0U;

    if( cc != NULL )
    {
        if( (cc->t5t.ccLen != NDEF_T5T_CC_LEN_8_BYTES) && (cc->t5t.ccLen != NDEF_T5T_CC_LEN_4_BYTES) )
        {
            return ERR_PARAM;
        }
        (void)ST_MEMCPY(&ctx->cc, cc, sizeof(ndefCapabilityContainer));
    }
    else
    {
        /* Try to find the appropriate cc values */
        ctx->cc.t5t.magicNumber  = NDEF_T5T_CC_MAGIC_1_BYTE_ADDR_MODE; /* E1 */
        ctx->cc.t5t.majorVersion = 1U;
        ctx->cc.t5t.minorVersion = 0U;
        ctx->cc.t5t.readAccess   = 0U;
        ctx->cc.t5t.writeAccess  = 0U;
        ctx->cc.t5t.lockBlock    = false;
        ctx->cc.t5t.specialFrame = false;
        ctx->cc.t5t.memoryLen    = 0U;
        ctx->cc.t5t.mlenOverflow = false;

        result = ndefT5TPollerReadMultipleBlocks(ctx, 0U, 0U, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvdLen);
        ctx->cc.t5t.multipleBlockRead = (result ==  ERR_NONE) ? true : false;

        /* Try to retrieve the tag's size using getSystemInfo and GetExtSystemInfo */

        if ( (ctx->subCtx.t5t.sysInfoSupported==true) && (ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags)!=0U) )
        {
            ctx->cc.t5t.memoryLen =  (uint16_t)((ctx->subCtx.t5t.sysInfo.numberOfBlock  * ctx->subCtx.t5t.sysInfo.blockSize) / NDEF_T5T_MLEN_DIVIDER);

            if( (options & NDEF_T5T_FORMAT_OPTION_NFC_FORUM) == NDEF_T5T_FORMAT_OPTION_NFC_FORUM ) /* NFC Forum format */
            {
                if( ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING )
                {
                    ctx->cc.t5t.ccLen =  NDEF_T5T_CC_LEN_8_BYTES;
                }
                if( ctx->cc.t5t.memoryLen > 0U )
                {
                    ctx->cc.t5t.memoryLen--; /* remove CC area from memory len */
                }
            }
            else /* Android format */
            {
                ctx->cc.t5t.ccLen = NDEF_T5T_CC_LEN_4_BYTES;
                 if( ctx->cc.t5t.memoryLen >= NDEF_T5T_MAX_MLEN_1_BYTE_ENCODING )
                {
                    ctx->cc.t5t.mlenOverflow = true;
                    ctx->cc.t5t.memoryLen    = 0xFFU;
                }
            }

            if( !ctx->subCtx.t5t.legacySTHighDensity && (ctx->subCtx.t5t.sysInfo.numberOfBlock > NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR) )
            {
                ctx->cc.t5t.magicNumber = NDEF_T5T_CC_MAGIC_2_BYTE_ADDR_MODE; /* E2 */
            }
        }
        else
        {
            return ERR_REQUEST;
        }
    }

    result = ndefT5TWriteCC(ctx);
    if( result != ERR_NONE )
    {
        /* If write fails, try to use special frame if not yet used */
        if( !ctx->cc.t5t.specialFrame )
        {
            platformDelay(20U); /* Wait to be sure that previous command has ended */
            ctx->cc.t5t.specialFrame = true; /* Add option flag */
            result = ndefT5TWriteCC(ctx);
            if( result != ERR_NONE )
            {
                ctx->cc.t5t.specialFrame = false; /* Add option flag */
                return result;
            }
        }
        else
        {
           return result;
        }
    }
    /* Update info about current NDEF */

    ctx->subCtx.t5t.TlvNDEFOffset = ctx->cc.t5t.ccLen;

    result = ndefT5TPollerWriteBytes(ctx, ctx->subCtx.t5t.TlvNDEFOffset, emptyNDEF, sizeof(emptyNDEF) );
    if (result == ERR_NONE)
    {
        /* Update info about current NDEF */
        ctx->messageOffset = (uint32_t)ctx->cc.t5t.ccLen + 0x02U;
        ctx->state         = NDEF_STATE_INITIALIZED;
    }
    return result;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerCheckPresence(ndefContext *ctx)
{
    ReturnCode          ret;
    uint16_t            blockAddr;
    uint16_t            rcvLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    blockAddr = 0U;

    ret = ndefT5TPollerReadSingleBlock( ctx, blockAddr, ctx->subCtx.t5t.txrxBuf, (uint16_t)sizeof(ctx->subCtx.t5t.txrxBuf), &rcvLen );

    return ret;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen)
{
    uint32_t            lLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if ( ctx->state == NDEF_STATE_INVALID )
    {
        return ERR_WRONG_STATE;
    }

    lLen = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;

    if( (messageLen + ctx->subCtx.t5t.TlvNDEFOffset + NDEF_T5T_TLV_T_LEN + lLen) > (ctx->areaLen + ctx->cc.t5t.ccLen) )
    {
        return ERR_NOMEM;
    }
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode           ret;
    uint32_t             lLen;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( (ctx->state != NDEF_STATE_INITIALIZED) && (ctx->state != NDEF_STATE_READWRITE) )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.4: reset L-Field to 0 */
    ret = ndefT5TPollerWriteRawMessageLen(ctx, 0U);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }

    lLen                = ( messageLen > NDEF_SHORT_VFIELD_MAX_LEN) ? NDEF_T5T_TLV_L_3_BYTES_LEN : NDEF_T5T_TLV_L_1_BYTES_LEN;
    ctx->messageOffset  = ctx->subCtx.t5t.TlvNDEFOffset;
    ctx->messageOffset += NDEF_T5T_TLV_T_LEN; /* T Len */
    ctx->messageOffset += lLen;               /* L Len */
    ctx->state          = NDEF_STATE_INITIALIZED;

    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode ndefT5TPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen)
{
    ReturnCode           ret;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->state != NDEF_STATE_INITIALIZED )
    {
        return ERR_WRONG_STATE;
    }

    /* TS T5T v1.0 7.5.3.6 & 7.5.3.7: update L-Field and write Terminator TLV */
    ret = ndefT5TPollerWriteRawMessageLen(ctx, messageLen);
    if( ret != ERR_NONE )
    {
        /* Conclude procedure */
        ctx->state = NDEF_STATE_INVALID;
        return ret;
    }
    ctx->messageLen = messageLen;
    ctx->state      = (ctx->messageLen == 0U) ? NDEF_STATE_INITIALIZED : NDEF_STATE_READWRITE;
    return ERR_NONE;
}

/*******************************************************************************/
static ReturnCode ndefT5TPollerWriteSingleBlock(ndefContext *ctx, uint16_t blockNum, const uint8_t* wrData)
{
    ReturnCode                ret;
    uint8_t                   flags;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    flags = ctx->cc.t5t.specialFrame ? ((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT | (uint8_t)RFAL_NFCV_REQ_FLAG_OPTION): (uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT;

    if( ctx->subCtx.t5t.legacySTHighDensity )
    {
        ret = rfalST25xVPollerM24LRWriteSingleBlock(flags, ctx->subCtx.t5t.pAddressedUid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
    }
    else
    {
        if( blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
        {
            ret = rfalNfcvPollerWriteSingleBlock(flags, ctx->subCtx.t5t.pAddressedUid, (uint8_t)blockNum, wrData, ctx->subCtx.t5t.blockLen);
        }
        else
        {
            ret = rfalNfcvPollerExtendedWriteSingleBlock(flags, ctx->subCtx.t5t.pAddressedUid, blockNum, wrData, ctx->subCtx.t5t.blockLen);
        }
    }

    return ret;
}

/*******************************************************************************/
static ReturnCode ndefT5TPollerReadMultipleBlocks(ndefContext *ctx, uint16_t firstBlockNum, uint8_t numOfBlocks, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
    ReturnCode                ret;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->subCtx.t5t.legacySTHighDensity )
    {

        ret = rfalST25xVPollerM24LRReadMultipleBlocks((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
    }
    else
    {
        if( firstBlockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
        {
            ret = rfalNfcvPollerReadMultipleBlocks((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, (uint8_t)firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
        }
        else
        {
            ret = rfalNfcvPollerExtendedReadMultipleBlocks((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, firstBlockNum, numOfBlocks, rxBuf, rxBufLen, rcvLen);
        }
    }

    return ret;
}

#endif /* NDEF_FEATURE_ALL */

/*******************************************************************************/
static ReturnCode ndefT5TPollerReadSingleBlock(ndefContext *ctx, uint16_t blockNum, uint8_t *rxBuf, uint16_t rxBufLen, uint16_t *rcvLen)
{
    ReturnCode                ret;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( ctx->subCtx.t5t.legacySTHighDensity )
    {

        ret = rfalST25xVPollerM24LRReadSingleBlock((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, blockNum, rxBuf, rxBufLen, rcvLen);
    }
    else
    {
        if( blockNum < NDEF_T5T_MAX_BLOCK_1_BYTE_ADDR )
        {
            ret = rfalNfcvPollerReadSingleBlock((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, (uint8_t)blockNum, rxBuf, rxBufLen, rcvLen);
        }
        else
        {
            ret = rfalNfcvPollerExtendedReadSingleBlock((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, blockNum, rxBuf, rxBufLen, rcvLen);
        }
    }

    return ret;
}

/*******************************************************************************/
static ReturnCode ndefT5TGetSystemInformation(ndefContext *ctx, bool extended)
{
    ReturnCode                ret;
    uint8_t                   rxBuf[NDEF_T5T_SYSINFO_MAX_LEN];
    uint16_t                  rcvLen;
    uint8_t*                  resp;

    if( (ctx == NULL) || !ndefT5TisT5TDevice(&ctx->device) )
    {
        return ERR_PARAM;
    }

    if( extended )
    {
        ret = rfalNfcvPollerExtendedGetSystemInformation((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT, ctx->subCtx.t5t.pAddressedUid, (uint8_t)RFAL_NFCV_SYSINFO_REQ_ALL, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
    }
    else
    {
        ret = rfalNfcvPollerGetSystemInformation(ctx->subCtx.t5t.legacySTHighDensity ? ((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT | (uint8_t)RFAL_NFCV_REQ_FLAG_PROTOCOL_EXT) : ((uint8_t)RFAL_NFCV_REQ_FLAG_DEFAULT), ctx->subCtx.t5t.pAddressedUid, rxBuf, (uint16_t)sizeof(rxBuf), &rcvLen);
    }

    if( ret != ERR_NONE )
    {
        return ret;
    }

    /* FIXME check buf rcvLen */
    resp = &rxBuf[0U];
    /* skip Flags */
    resp++;
    /* get Info flags */
    ctx->subCtx.t5t.sysInfo.infoFlags = *resp;
    resp++;
    if( extended && (ndefT5TSysInfoLenValue(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
    {
        return ERR_PROTO;
    }
    /* get UID */
    (void)ST_MEMCPY(ctx->subCtx.t5t.sysInfo.UID, resp, RFAL_NFCV_UID_LEN);
    resp = &resp[RFAL_NFCV_UID_LEN];
    if( ndefT5TSysInfoDFSIDPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U)
    {
        ctx->subCtx.t5t.sysInfo.DFSID = *resp;
        resp++;
    }
    if( ndefT5TSysInfoAFIPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        ctx->subCtx.t5t.sysInfo.AFI = *resp;
        resp++;
    }
    if( ndefT5TSysInfoMemSizePresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        if ( ctx->subCtx.t5t.legacySTHighDensity || extended )
        {
            /* LRIS64K/M24LR16/M24LR64 */
            ctx->subCtx.t5t.sysInfo.numberOfBlock =  *resp;
            resp++;
            ctx->subCtx.t5t.sysInfo.numberOfBlock |= (((uint16_t)*resp) << 8U);
            resp++;
        }
        else
        {
            ctx->subCtx.t5t.sysInfo.numberOfBlock = *resp;
            resp++;
        }
        ctx->subCtx.t5t.sysInfo.blockSize = *resp;
        resp++;
        /* Add 1 to get real values*/
        ctx->subCtx.t5t.sysInfo.numberOfBlock++;
        ctx->subCtx.t5t.sysInfo.blockSize++;
    }
    if( ndefT5TSysInfoICRefPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U )
    {
        ctx->subCtx.t5t.sysInfo.ICRef = *resp;
        resp++;
    }
    if( extended && (ndefT5TSysInfoCmdListPresent(ctx->subCtx.t5t.sysInfo.infoFlags) != 0U) )
    {
        ctx->subCtx.t5t.sysInfo.supportedCmd[0U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[1U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[2U] = *resp;
        resp++;
        ctx->subCtx.t5t.sysInfo.supportedCmd[3U] = *resp;
        resp++;
    }
    return ERR_NONE;
}

#endif /* RFAL_FEATURE_NFCV */
