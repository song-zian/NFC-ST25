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
 *  \brief Provides NDEF methods and definitions to access NFC Forum Tags
 *
 *  NDEF provides several functionalities required to perform NFC NDEF activities. 
 *  <br>The NDEF encapsulates the different tag technologies (T2T, T3T, T4AT, T4BT, T5T)
 *  into a common and easy to use interface.
 *  
 *  It provides interfaces to Detect, Read, Write and Format NDEF. 
 *  
 *  The most common interfaces are:
 *    <br>&nbsp; ndefPollerContextInitialization()
 *    <br>&nbsp; ndefPollerNdefDetect()
 *    <br>&nbsp; ndefPollerReadRawMessage()
 *    <br>&nbsp; ndefPollerWriteRawMessage()
 *    <br>&nbsp; ndefPollerTagFormat()
 *    <br>&nbsp; ndefPollerWriteMessage()
 *
 *
 *  An NDEF read usage example is provided here: \ref ndef_example_read.c
 *  \example ndef_example_read.c
 *
 *  An NDEF write usage example is provided here: \ref ndef_example_write.c
 *  \example ndef_example_write.c
 *
 * \addtogroup NDEF
 * @{
 *
 */


#ifndef NDEF_POLLER_H
#define NDEF_POLLER_H

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "platform.h"
#include "st_errno.h"
#include "rfal_nfca.h"
#include "rfal_nfcb.h"
#include "rfal_nfcf.h"
#include "rfal_nfcv.h"
#include "rfal_nfc.h"
#include "rfal_isoDep.h"
#include "rfal_t4t.h"
#include "ndef_record.h"
#include "ndef_message.h"


/*
 ******************************************************************************
 * GLOBAL DEFINES
 ******************************************************************************
 */

#define NDEF_CC_BUF_LEN             17U                                                /*!< CC buffer len. Max len = 17 in case of T4T v3                */
#define NDEF_NFCV_SUPPORTED_CMD_LEN  4U                                                /*!< Ext sys info supported commands list len                     */

#define NDEF_SHORT_VFIELD_MAX_LEN  254U                                                /*!< Max V-field length for 1-byte Lengh encoding                 */
#define NDEF_TERMINATOR_TLV_LEN      1U                                                /*!< Terminator TLV size                                          */
#define NDEF_TERMINATOR_TLV_T     0xFEU                                                /*!< Terminator TLV T=FEh                                         */

#define NDEF_T2T_READ_RESP_SIZE     16U                                                /*!< Size of the READ response i.e. four blocks                   */

#define NDEF_T3T_BLOCK_SIZE         16U                                                /*!< size for a block in t3t                                      */
#define NDEF_T3T_MAX_NB_BLOCKS       4U                                                /*!< size for a block in t3t                                      */
#define NDEF_T3T_MAX_RX_SIZE      ((NDEF_T3T_BLOCK_SIZE*NDEF_T3T_MAX_NB_BLOCKS) + 16U) /*!< size for receive 4 blocks of 16 + UID + HEADER + CHECKSUM    */
#define NDEF_T3T_MAX_TX_SIZE      ((NDEF_T3T_BLOCK_SIZE*NDEF_T3T_MAX_NB_BLOCKS) + 16U) /*!< size for send Check 4 blocks of 16 + UID + HEADER + CHECKSUM */

#define NDEF_T5T_TxRx_BUFF_HEADER_SIZE        1U                                       /*!< Request Flags/Responses Flags size                           */
#define NDEF_T5T_TxRx_BUFF_FOOTER_SIZE        2U                                       /*!< CRC size                                                     */

#define NDEF_T5T_TxRx_BUFF_SIZE               \
          (32U +  NDEF_T5T_TxRx_BUFF_HEADER_SIZE + NDEF_T5T_TxRx_BUFF_FOOTER_SIZE)     /*!< T5T working buffer size                                      */

/*
 ******************************************************************************
 * GLOBAL MACROS
 ******************************************************************************
 */

#define ndefBytes2Uint16(hiB, loB)          ((uint16_t)((((uint32_t)(hiB)) << 8U) | ((uint32_t)(loB))))                                                  /*!< convert 2 bytes to a u16 */

#define ndefMajorVersion(V)                 ((uint8_t)((V) >>  4U))    /*!< Get major version */
#define ndefMinorVersion(V)                 ((uint8_t)((V) & 0xFU))    /*!< Get minor version */

/*
 ******************************************************************************
 * NDEF FEATURES CONFIGURATION
 ******************************************************************************
 */

#define NDEF_FEATURE_ALL                       true    /*!< Enable/Disable NDEF support for full features (write NDEF, format, ...)   */

/*
 ******************************************************************************
 * GLOBAL TYPES
 ******************************************************************************
 */

/*! NDEF device type */
typedef enum {
    NDEF_DEV_NONE          = 0x00U,                            /*!< Device type undef                                  */
    NDEF_DEV_T1T           = 0x01U,                            /*!< Device type T1T                                    */
    NDEF_DEV_T2T           = 0x02U,                            /*!< Device type T2T                                    */
    NDEF_DEV_T3T           = 0x03U,                            /*!< Device type T3T                                    */
    NDEF_DEV_T4T           = 0x04U,                            /*!< Device type T4AT                                   */
    NDEF_DEV_T5T           = 0x05U,                            /*!< Device type T5T                                    */
} ndefDeviceType;

/*! NDEF states  */
typedef enum {
    NDEF_STATE_INVALID     = 0x00U,                            /*!< Invalid state (e.g. no CC)                         */
    NDEF_STATE_INITIALIZED = 0x01U,                            /*!< State Initialized (no NDEF message)                */
    NDEF_STATE_READWRITE   = 0x02U,                            /*!< Valid NDEF found. Read/Write capability            */
    NDEF_STATE_READONLY    = 0x03U,                            /*!< Valid NDEF found. Read only                        */
} ndefState;

/*! NDEF Information */
typedef struct {
    uint8_t                  majorVersion;                     /*!< Major version                                      */
    uint8_t                  minorVersion;                     /*!< Minor version                                      */
    uint32_t                 areaLen;                          /*!< Area Len for NDEF storage                          */
    uint32_t                 areaAvalableSpaceLen;             /*!< Remaining Space in case a propTLV is present       */
    uint32_t                 messageLen;                       /*!< NDEF message Len                                   */
    ndefState                state;                            /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
} ndefInfo;

/*! NFCV (Extended) System Information  */
typedef struct {
    uint16_t                 numberOfBlock;                    /*!< Number of block                                    */
    uint8_t                  UID[RFAL_NFCV_UID_LEN];           /*!< UID Value                                          */
    uint8_t                  supportedCmd[NDEF_NFCV_SUPPORTED_CMD_LEN];/*!< Supported Commands list                    */
    uint8_t                  infoFlags;                        /*!< Information flags                                  */
    uint8_t                  DFSID;                            /*!< DFSID Value                                        */
    uint8_t                  AFI;                              /*!< AFI Value                                          */
    uint8_t                  blockSize;                        /*!< Block Size Value                                   */
    uint8_t                  ICRef;                            /*!< IC Reference                                       */
} ndefSystemInformation;

/*! T1T Capability Container  */
typedef struct {
    uint8_t                  magicNumber;                      /*!< Magic number e.g. E1h                              */
    uint8_t                  majorVersion;                     /*!< Major version i.e 1                                */
    uint8_t                  minorVersion;                     /*!< Minor version i.e 2                                */
    uint16_t                 tagMemorySize;                    /*!< Tag Memory Size (TMS)                              */
    uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
    uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
} ndefCapabilityContainerT1T;

/*! T2T Capability Container  */
typedef struct {
    uint8_t                  magicNumber;                      /*!< Magic number e.g. E1h                              */
    uint8_t                  majorVersion;                     /*!< Major version i.e 1                                */
    uint8_t                  minorVersion;                     /*!< Minor version i.e 2                                */
    uint8_t                  size;                             /*!< Size. T2T_Area_Size = Size * 8                     */
    uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
    uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
} ndefCapabilityContainerT2T;

/*! T3T Attribute info block  */
typedef struct {
    uint8_t                  majorVersion;                     /*!< Major version i.e 1                                */
    uint8_t                  minorVersion;                     /*!< Minor version i.e 2                                */
    uint8_t                  nbR;                              /*!< Nbr: number of blocks read in one CHECK cmd        */
    uint8_t                  nbW;                              /*!< Nbr: number of blocks written in one UPDATE cmd    */
    uint16_t                 nMaxB;                            /*!< NmaxB: max number of blocks for NDEF data          */
    uint8_t                  writeFlag;                        /*!< WriteFlag indicates completion of previous NDEF    */
    uint8_t                  rwFlag;                           /*!< RWFlag indicates whether the NDEF can be updated   */
    uint32_t                 Ln;                               /*!< Ln size of the actual stored NDEF data in bytes    */
} ndefAttribInfoBlockT3T;

/*! T4T Capability Container  */
typedef struct {
    uint16_t                 ccLen;                            /*!< CCFILE Len                                         */
    uint8_t                  vNo;                              /*!< Mapping version                                    */
    uint16_t                 mLe;                              /*!< Max data size that can be read using a ReadBinary  */
    uint16_t                 mLc;                              /*!< Max data size that can be sent using a single cmd  */
    uint8_t                  fileId[2];                        /*!< NDEF File Identifier                               */
    uint32_t                 fileSize;                         /*!< NDEF File Size                                     */
    uint8_t                  readAccess;                       /*!< NDEF File READ access condition                    */
    uint8_t                  writeAccess;                      /*!< NDEF File WRITE access condition                   */
} ndefCapabilityContainerT4T;

/*! T5T Capability Container  */
typedef struct {
    uint8_t                  ccLen;                            /*!< CC Len                                             */
    uint8_t                  magicNumber;                      /*!< Magic number i.e. E1h or E2h                       */
    uint8_t                  majorVersion;                     /*!< Major version i.e 1                                */
    uint8_t                  minorVersion;                     /*!< Minor version i.e 0                                */
    uint8_t                  readAccess;                       /*!< NDEF READ access condition                         */
    uint8_t                  writeAccess;                      /*!< NDEF WRITE access condition                        */
    bool                     specialFrame;                     /*!< Use Special Frames for Write-alike commands        */
    bool                     lockBlock;                        /*!< (EXTENDED_)LOCK_SINGLE_BLOCK supported             */
    bool                     multipleBlockRead;                /*!< (EXTENDED_)READ_MULTIPLE_BLOCK supported           */
    bool                     mlenOverflow;                     /*!< memory size exceeds 2040 bytes (Android)           */
    uint16_t                 memoryLen;                        /*!< MLEN (Memory Len). T5T_Area size = 8 * MLEN (bytes)*/
} ndefCapabilityContainerT5T;

/*! Generic Capability Container  */
typedef union {
    ndefCapabilityContainerT1T   t1t;                          /*!< T1T Capability Container                           */
    ndefCapabilityContainerT2T   t2t;                          /*!< T2T Capability Container                           */
    ndefAttribInfoBlockT3T       t3t;                          /*!< T3T Attribute Information Block                    */
    ndefCapabilityContainerT4T   t4t;                          /*!< T4T Capability Container                           */
    ndefCapabilityContainerT5T   t5t;                          /*!< T5T Capability Container                           */
} ndefCapabilityContainer;

/*! NDEF T1T sub context structure */
typedef struct {
    void * rfu;                                                /*!< RFU                                                */
} ndefT1TContext;

/*! NDEF T2T sub context structure */
typedef struct {
    uint8_t                     currentSecNo;                      /*!< Current sector number                          */
    uint8_t                     cacheBuf[NDEF_T2T_READ_RESP_SIZE]; /*!< Cache buffer                                   */
    uint32_t                    cacheAddr;                         /*!< Address of cached data                         */
    uint32_t                    offsetNdefTLV;                     /*!< NDEF TLV message offset                        */
} ndefT2TContext;

/*! NDEF T3T sub context structure */
typedef struct {
    uint8_t                      txbuf[NDEF_T3T_MAX_TX_SIZE];         /*!< Tx buffer dedicated for T3T internal operations         */
    uint8_t                      rxbuf[NDEF_T3T_MAX_RX_SIZE];         /*!< Rx buffer dedicated for T3T internal operations         */
    rfalNfcfBlockListElem        listBlocks[NDEF_T3T_MAX_NB_BLOCKS];  /*!< block number list for T3T internal operations           */
} ndefT3TContext;

/*! NDEF T4T sub context structure */
typedef struct {
    uint8_t                      curMLe;                       /*!< Current MLe. Default Fh until CC file is read      */
    uint8_t                      curMLc;                       /*!< Current MLc. Default Dh until CC file is read      */
    bool                         mv1Flag;                      /*!< Mapping version 1 flag                             */
    rfalIsoDepApduBufFormat      cApduBuf;                     /*!< Command-APDU buffer                                */
    rfalIsoDepApduBufFormat      rApduBuf;                     /*!< Response-APDU buffer                               */
    rfalT4tRApduParam            respAPDU;                     /*!< Response-APDU params                               */
    rfalIsoDepBufFormat          tmpBuf;                       /*!< I-Block temporary buffer                           */
    uint16_t                     rApduBodyLen;                 /*!< Response Body Len                                  */
} ndefT4TContext;

/*! NDEF T5T sub context structure */
typedef struct {
    uint8_t *                    pAddressedUid;                /*!< Pointer to UID in Addr mode or NULL selected one   */
    uint32_t                     TlvNDEFOffset;                /*!< NDEF TLV message offset                            */
    uint8_t                      blockLen;                     /*!< T5T BLEN parameter                                 */
    ndefSystemInformation        sysInfo;                      /*!< System Information (when supported)                */
    bool                         sysInfoSupported;             /*!< System Information Supported flag                  */
    bool                         legacySTHighDensity;          /*!< Legacy ST High Density flag                        */
    uint8_t                      txrxBuf[NDEF_T5T_TxRx_BUFF_SIZE];  /*!< Tx Rx Buffer                                  */
} ndefT5TContext;

/*! NDEF context structure */
typedef struct {
    rfalNfcDevice                device;                       /*!< ndef Device                                        */
    ndefState                    state;                        /*!< Tag state e.g. NDEF_STATE_INITIALIZED              */
    ndefCapabilityContainer      cc;                           /*!< Capability Container                               */
    uint32_t                     messageLen;                   /*!< NDEF message len                                   */
    uint32_t                     messageOffset;                /*!< NDEF message offset                                */
    uint32_t                     areaLen;                      /*!< Area Len for NDEF storage                          */
    uint8_t                      ccBuf[NDEF_CC_BUF_LEN];       /*!< buffer for CC                                      */
    const struct ndefPollerWrapperStruct*
                                 ndefPollWrapper;              /*!< pointer to array of function for wrapper           */
    union {
        ndefT1TContext t1t;                                    /*!< T1T context                                        */
        ndefT2TContext t2t;                                    /*!< T2T context                                        */
        ndefT3TContext t3t;                                    /*!< T3T context                                        */
#if RFAL_FEATURE_T4T
        ndefT4TContext t4t;                                    /*!< T4T context                                        */
#endif
        ndefT5TContext t5t;                                    /*!< T5T context                                        */
    } subCtx;                                                  /*!< Sub-context union                                  */

} ndefContext;

/*! Wrapper struture to hold the function pointers on each tag type  */
typedef struct ndefPollerWrapperStruct
{
    ReturnCode (* pollerContextInitialization)(ndefContext *ctx, const rfalNfcDevice *dev);                             /*!< ContextInitialization function pointer                 */
    ReturnCode (* pollerNdefDetect)(ndefContext *ctx, ndefInfo *info);                                                  /*!< NdefDetect function pointer                            */
    ReturnCode (* pollerReadBytes)(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);   /*!< Read function pointer                                  */
    ReturnCode (* pollerReadRawMessage)(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen);            /*!< ReadRawMessage function pointer                        */
#if NDEF_FEATURE_ALL
    ReturnCode (* pollerWriteBytes)(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len);               /*!< Write function pointer                                 */
    ReturnCode (* pollerWriteRawMessage)(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);                        /*!< WriteRawMessage function pointer                       */
    ReturnCode (* pollerTagFormat)(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);              /*!< TagFormat function pointer                             */
    ReturnCode (* pollerWriteRawMessageLen)(ndefContext *ctx, uint32_t rawMessageLen);                                  /*!< WriteRawMessageLen function pointer                    */
    ReturnCode (* pollerCheckPresence)(ndefContext *ctx);                                                               /*!< CheckPresence function pointer                         */
    ReturnCode (* pollerCheckAvailableSpace)(const ndefContext *ctx, uint32_t messageLen);                              /*!< CheckAvailableSpace function pointer                   */
    ReturnCode (* pollerBeginWriteMessage)(ndefContext *ctx, uint32_t messageLen);                                      /*!< BeginWriteMessage function pointer                     */
    ReturnCode (* pollerEndWriteMessage)(ndefContext *ctx, uint32_t messageLen);                                        /*!< EndWriteMessage function pointer                       */
#endif /* NDEF_FEATURE_ALL */
} ndefPollerWrapper;

/*
 ******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 ******************************************************************************
 */

/*!
 *****************************************************************************
 * \brief Handle NDEF context activation
 *
 * This method performs the initialization of the NDEF context.
 * It must be called after a successful
 * anti-collision procedure and prior to any NDEF procedures such as NDEF
 * detection procedure.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   dev    : ndef Device
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerContextInitialization(ndefContext *ctx, const rfalNfcDevice *dev);


/*!
 *****************************************************************************
 * \brief NDEF Detection procedure
 *
 * This method performs the NDEF Detection procedure
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  info   : ndef Information (optional parameter, NULL may be used when no NDEF Information is needed)
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : Detection failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerNdefDetect(ndefContext *ctx, ndefInfo *info);


/*!
 *****************************************************************************
 * \brief Read data
 *
 * This method reads arbitrary length data
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to start reading data
 * \param[in]   len    : requested len
 * \param[out]  buf    : buffer to place the data read from the tag
 * \param[out]  rcvdLen: received length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerReadBytes(ndefContext *ctx, uint32_t offset, uint32_t len, uint8_t *buf, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief  Write data
 *
 * This method writes arbitrary length data from the current selected file
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   offset : file offset of where to start writing data
 * \param[in]   buf    : data to write
 * \param[in]   len    : buf len
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteBytes(ndefContext *ctx, uint32_t offset, const uint8_t *buf, uint32_t len);


/*!
 *****************************************************************************
 * \brief Read raw NDEF message
 *
 * This method reads a raw NDEF message.
 * Prior to NDEF Read procedure, a successful ndefPollerNdefDetect()
 * has to be performed.
 *
 *
 * \param[in]   ctx    : ndef Context
 * \param[out]  buf    : buffer to place the NDEF message
 * \param[in]   bufLen : buffer length
 * \param[out]  rcvdLen: received length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : read failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerReadRawMessage(ndefContext *ctx, uint8_t *buf, uint32_t bufLen, uint32_t *rcvdLen);


/*!
 *****************************************************************************
 * \brief Write raw NDEF message
 *
 * This method writes a raw NDEF message.
 * Prior to NDEF Write procedure, a successful ndefPollerNdefDetect()
 * has to be performed.
 *
 * \param[in]   ctx    : ndef Context
 * \param[in]   buf    : raw message buffer
 * \param[in]   bufLen : buffer length
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteRawMessage(ndefContext *ctx, const uint8_t *buf, uint32_t bufLen);


/*!
 *****************************************************************************
 * \brief Format Tag
 *
 * This method format a tag to make it ready for NDEF storage.
 * cc and options parameters usage is described in each technology method
 * (ndefT[2345]TPollerTagFormat)
 *
 * \param[in]   ctx     : ndef Context
 * \param[in]   cc      : Capability Container
 * \param[in]   options : specific flags
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerTagFormat(ndefContext *ctx, const ndefCapabilityContainer *cc, uint32_t options);


/*!
 *****************************************************************************
 * \brief Write NDEF message length
 *
 * This method writes the NLEN field
 *
 * \param[in]   ctx          : ndef Context
 * \param[in]   rawMessageLen: len
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteRawMessageLen(ndefContext *ctx, uint32_t rawMessageLen);


 /*!
 *****************************************************************************
 * \brief Write an NDEF message
 *
 * Write the NDEF message to the tag
 *
 * \param[in] ctx:     NDEF Context
 * \param[in] message: Message to write
 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_REQUEST      : write failed
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerWriteMessage(ndefContext *ctx, const ndefMessage* message);


/*!
 *****************************************************************************
 * \brief Check Presence
 *
 * This method check whether an NFC tag is still present in the operating field
 *
 * \param[in]   ctx    : ndef Context

 *
 * \return ERR_WRONG_STATE  : RFAL not initialized or mode not set
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_PROTO        : Protocol error
 * \return ERR_NONE         : No error
 *****************************************************************************
 */
ReturnCode ndefPollerCheckPresence(ndefContext *ctx);


/*!
 *****************************************************************************
 * \brief Check Available Space
 *
 * This method check whether a NFC tag has enough space to write a message of a given length
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerCheckAvailableSpace(const ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief Begin Write Message
 *
 * This method sets the L-field to 0 (T1T, T2T, T4T, T5T) or set the WriteFlag (T3T) and sets the message offset to the proper value according to messageLen
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerBeginWriteMessage(ndefContext *ctx, uint32_t messageLen);


/*!
 *****************************************************************************
 * \brief End Write Message
 *
 * This method updates the L-field value after the message has been written and resets the WriteFlag (for T3T only)
 *
 * \param[in]   ctx       : ndef Context
 * \param[in]   messageLen: message length
 *
 * \return ERR_PARAM        : Invalid parameter
 * \return ERR_NOMEM        : not enough space
 * \return ERR_NONE         : Enough space for message of messageLen length
 *****************************************************************************
 */
ReturnCode ndefPollerEndWriteMessage(ndefContext *ctx, uint32_t messageLen);


#endif /* NDEF_POLLER_H */

/**
  * @}
  *
  */
