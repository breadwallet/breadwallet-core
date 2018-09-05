//
//  BREthereumLES.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 5/01/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BR_Ethereum_LES_h
#define BR_Ethereum_LES_h

#include <inttypes.h>
#include "BRKey.h"
#include "BRArray.h"
#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumBlockChain.h"

#include "BREthereumProvision.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK:  LES Node Config

/**
 * A LES Node Config is used to save a node for use when reestablishing LES.  See the typedef
 * BREthereumLESCallbackSaveNodes.
 */
typedef struct BREthereumLESNodeConfigRecord *BREthereumLESNodeConfig;

extern void
lesNodeConfigRelease (BREthereumLESNodeConfig config);

extern BREthereumHash
lesNodeConfigGetHash (BREthereumLESNodeConfig config);

extern BRRlpItem
lesNodeConfigEncode (BREthereumLESNodeConfig config,
                     BRRlpCoder coder);

extern BREthereumLESNodeConfig
lesNodeConfigDecode (BRRlpItem item,
                     BRRlpCoder coder);

/*!
 * @typedef BREthereumLES
 *
 * @abstract
 * An instance to handle LES functionalty.
 */
typedef struct BREthereumLESRecord *BREthereumLES;

/**
 * An opaque type for a Node.  Only used in the announce callback to identify what node produced
 * the hea
 */
typedef void *BREthereumLESNodeReference;

/*!
 *@typedef BREthereumLESStatus
 *
 * @abstract
 * An enumeration for deteremining the error that occured when submitting messages using LES
 */
typedef enum {
    LES_SUCCESS                   = 0x00,  // No error was generated after submtting a message using LES
    LES_ERROR_NETWORK_UNREACHABLE = 0x01,  // Error is thrown when the LES context can not connect to the ethereum network
    LES_ERROR_UNKNOWN             = 0x02   // Error is thrown but it's unknown what caused it.
                                           // NOT_CONNECTED?
} BREthereumLESStatus;

/*!
 * @typedef BREthereumLESCallbackContext
 *
 * @abstract
 * The context to use for handling LES callbacks, such as 'announce' and 'status'
 */
typedef void* BREthereumLESCallbackContext;

/*!
 * @typedef BREthereumLESCallbackAnnounce
 *
 * @abstract
 * The callback to use for handling a LES 'Announce' message.
 */
typedef void
(*BREthereumLESCallbackAnnounce) (BREthereumLESCallbackContext context,
                                  BREthereumHash headHash,
                                  uint64_t headNumber,
                                  UInt256 headTotalDifficulty,
                                  uint64_t reorgDepth);

/**
 * The callback to use for Node/Peer status message - announces the headHash and headNumber for
 * the peer.
 *
 * TODO: Might need a `peer ID` or let the NodeManager evaluate different peers.
 *
 */
typedef void
(*BREthereumLESCallbackStatus) (BREthereumLESCallbackContext context,
                                BREthereumHash headHash,
                                uint64_t headNumber);

/**
 * The callback to use for saving nodes
 */
typedef void
(*BREthereumLESCallbackSaveNodes) (BREthereumLESCallbackContext context,
                                   BRArrayOf(BREthereumLESNodeConfig) nodes);

/*!
 * @function lesCreate
 *
 * @abstract
 * Create an instance to handle the LES interface.  Will connect to the provided `network` and
 * begin getting block headers.
 *
 * @param network
 * The network to connect with.
 *
 * @param announceContext
 * The context to use when handling a LES 'Announce' message
 *
 * @param announceCallback
 * The callback to use when handling a LES 'Announce' message
 *
 * ...
 *
 * @result
 * A new LES interface handler.
 */
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESCallbackContext callbackContext,
           BREthereumLESCallbackAnnounce callbackAnnounce,
           BREthereumLESCallbackStatus callbackStatus,
           BREthereumLESCallbackSaveNodes callbackSaveNodes,
           BREthereumHash headHash,
           uint64_t headNumber,
           UInt256 headTotalDifficulty,
           BREthereumHash genesisHash,
           BRArrayOf(BREthereumLESNodeConfig) configs);

/*!
 * @function lesRelease
 *
 * @abstract
 * Releases the les context
 */
extern void
lesRelease(BREthereumLES les);

extern void
lesStart (BREthereumLES les);

extern void
lesStop (BREthereumLES les);

/// MARK: LES Provision Callbacks

typedef void *BREthereumLESProvisionContext;

typedef void
(*BREthereumLESProvisionCallback) (BREthereumLESProvisionContext context,
                                   BREthereumLES les,
                                   BREthereumLESNodeReference node,
                                   BREthereumProvisionResult result);

/*!
 * @function lesProvideBlockHeaders
 *
 * @abstract
 * Make a LES Block Headers requests.
 *
 * @param context
 * The context to use for the callback.
 *
 * @param callback
 * The callback to use when handling each BREthereumBlockHeader.
 *
 * @param blockNumber
 * The starting blockNumber
 *
 * @param maxBlockCount
 * The maximum blocks to return.
 *
 * @param skip
 * The number of blocks to skip.  Should be '0' to get every block between `blockNumber` and
 * `blockNumber + maxBlockCount`.
 *
 * @param reverse
 * If ETHEREUM_BOOLEAN_TRUE the returned headers will be in reverse order; otherwise the
 * first header will have `blockNumber`.
 */
extern void
lesProvideBlockHeaders (BREthereumLES les,
                        BREthereumLESProvisionContext context,
                        BREthereumLESProvisionCallback callback,
                        uint64_t blockNumber,
                        uint32_t maxBlockCount,
                        uint64_t skip,
                        BREthereumBoolean reverse);

/*!
 * @function lesProvdeBlockBodies
 *
 * @abstract
 * Make a LES Block Bodies requests.
 *
 * @param les
 * @param context
 * @param callback
 * @param BREthereumHash
 */
extern void
lesProvideBlockBodies (BREthereumLES les,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BRArrayOf(BREthereumHash) blockHashes);

extern void
lesProvideBlockBodiesOne (BREthereumLES les,
                          BREthereumLESProvisionContext context,
                          BREthereumLESProvisionCallback callback,
                          BREthereumHash blockHash);

/**
 * @function lesProvideReceipts
 *
 * @param les
 * @param context
 * @param callback
 * @param BREthereumHash
 */
extern void
lesProvideReceipts (BREthereumLES les,
                    BREthereumLESProvisionContext context,
                    BREthereumLESProvisionCallback callback,
                    BRArrayOf(BREthereumHash) blockHashes);
extern void
lesProvideReceiptsOne (BREthereumLES les,
                       BREthereumLESProvisionContext context,
                       BREthereumLESProvisionCallback callback,
                       BREthereumHash blockHash);

/**
 * @function lesProvideAccountStates
 *
 * @param les
 * @param context
 * @param callback
 * @param address
 * @param BREthereumHash
 */
extern void
lesProvideAccountStates (BREthereumLES les,
                         BREthereumLESProvisionContext context,
                         BREthereumLESProvisionCallback callback,
                         BREthereumAddress address,
                         BRArrayOf(BREthereumHash) blockHashes,
                         BRArrayOf(uint64_t) blockNumbers);  // HACK

extern void
lesProvideAccountStatesOne (BREthereumLES les,
                            BREthereumLESProvisionContext context,
                            BREthereumLESProvisionCallback callback,
                            BREthereumAddress address,
                            BREthereumHash blockHash,
                            uint64_t blockNumber);  // HACK

/**
 * @function lesProvideTransactionStauts
 *
 * @param les
 * @param context
 * @param callback
 * @param BREthereumHash
 */
extern void
lesProvideTransactionStatus (BREthereumLES les,
                             BREthereumLESProvisionContext context,
                             BREthereumLESProvisionCallback callback,
                             BRArrayOf(BREthereumHash) transactionHashes);

extern void
lesProvideTransactionStatusOne (BREthereumLES les,
                                BREthereumLESProvisionContext context,
                                BREthereumLESProvisionCallback callback,
                                BREthereumHash transactionHash);

/**
 * @function lesSubmitTransaction
 *
 * @param les
 * @param context
 * @param callback
 * @param transaction
 */
extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESProvisionContext context,
                      BREthereumLESProvisionCallback callback,
                      BREthereumTransaction transaction);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_h */
