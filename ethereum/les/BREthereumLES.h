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
#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumBlockChain.h"
#include "BRKey.h"
#include "BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @typedef BREthereumLES
 *
 * @abstract
 * An instance to handle LES functionalty.
 */
typedef struct BREthereumLESContext *BREthereumLES;

/*!
 *@typedef BREthereumLESStatus
 *
 * @abstract
 * An enumeration for deteremining the error that occured when submitting messages using LES
 */
typedef enum {
    LES_SUCCESS  = 0x00,            // No error was generated after submtting a message using LES
    LES_NETWORK_UNREACHABLE = 0x01, // Error is thrown when the LES context can not connect to the ethereum network
    LES_UNKNOWN_ERROR = 0x02        // Error is thrown but it's unknown what caused it.
}BREthereumLESStatus;


/*!
 * @typedef BREthereumLESAnnounceContext
 *
 * @abstract
 * The context to use for handling a LES 'Announce' message
 */
typedef void* BREthereumLESAnnounceContext;

/*!
 * @typedef BREthereumLESAnnounceCallback
 *
 * @abstract
 * The callback to use for handling a LES 'Announce' message.
 */
typedef void
(*BREthereumLESAnnounceCallback) (BREthereumLESAnnounceContext context,
                                  BREthereumHash headHash,
                                  uint64_t headNumber,
                                  UInt256 headTotalDifficulty);


/*!
 * @function lesCreate
 *
 * @abstract
 * Allocated and configures all resources for a les handler
 *
 * @result
 * A new LES interface handler.
 */
extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESAnnounceContext announceContext,
           BREthereumLESAnnounceCallback announceCallback,
           BREthereumHash headHash,
           uint64_t headNumber,
           uint64_t headTotalDifficulty,
           BREthereumHash genesisHash);

/*!
 * @function lesStart
 *
 * @abstract
 * Allows a les handler to begin handling les messages
 *
 * Note: This is a blocking function. This function will wait until the handler is able to begin sending/receiving messages.
 * If the handler cannot connect to the Ethereum Network after given by the "maxWaitTime" parameter
 *
 * @param les
 * The les handler
 *
 * @param maxWaitTime
 * The maxiumum time (in seconds) the function should wait to allow the handler to connect to the ethereum network.
 *
 *
 * @result
 * ETHEREUM_BOOLEAN_TRUE, if the handler succesfully connected to the network, otherwise ETHEREUM_BOOLEAN_FALSE
 *
 */
extern BREthereumBoolean
 lesStart(BREthereumLES les, unsigned int maxWaitTime);
 
 
 /*!
 * @function lesStop
 *
 * @abstract
 * Notifies the given handler that it should stop handling mesasges
 *
 * Note: Currently, this function does not allow message to queue up. Thus, make sure you received the necessary responses before calling this function.
 *
 * @param les
 * The les handler
 *
 */
extern void
 lesStop(BREthereumLES les);


 /*!
 * @function lesStop
 *
 * @abstract
 * Notifies the given handler that it should stop handling mesasges
 *
 * Note: Currently, this function does not allow message to queue up. Thus, make sure you received the necessary responses before calling this function.
 *
 * @param les
 * The les handler
 *
 */
extern void
 lesStop(BREthereumLES les);


/*!
 * @function lesRelease
 *
 * @abstract
 * Releases the les context
 *
 * Note: If the function "lesStop" was not called before this function then "lesRelease" calls "lesStop". This function will wait until
 * all resources have been deallocated before returning.
 *
 */
extern void
lesRelease(BREthereumLES les);




///////
//
// LES Message functions
//
////


//
// LES GetBlockHeaders
//

/*!
 * @typedef BREthereumLESBlockHeadersContext
 *
 * @abstract
 * A context for the BlockHeaders callback
 */
typedef void* BREthereumLESBlockHeadersContext;

/*!
 * @typedef BREthereumLESBlockHeadersCallback
 *
 * @abstract
 * A callback for handling a BlockHeaders result.  Passed the `context` and `header`.
 */
typedef void
(*BREthereumLESBlockHeadersCallback) (BREthereumLESBlockHeadersContext context,
                                      BREthereumBlockHeader header);




/*!
 * @function lesGetBlockHeaders
 *
 * @abstract
 * Make a LES GetBlockHeaders requests.  The result will be an array of BREthereumBlockHeader;
 * the callback will be applied one-by-one to each header.
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
extern BREthereumLESStatus
lesGetBlockHeaders (BREthereumLES les,
                    BREthereumLESBlockHeadersContext context,
                    BREthereumLESBlockHeadersCallback callback,
                    uint64_t blockNumber,
                    size_t maxBlockCount,
                    uint64_t skip,
                    BREthereumBoolean reverse);

//
// LES GetBlockBodies
//
typedef void* BREthereumLESBlockBodiesContext;

typedef void
(*BREthereumLESBlockBodiesCallback) (BREthereumLESBlockBodiesContext context,
                                     BREthereumHash block,
                                     BREthereumTransaction transactions[],
                                     BREthereumBlockHeader ommers[]);

extern BREthereumLESStatus
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BREthereumHash blocks[]);

extern BREthereumLESStatus
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash block);



//
// LES GetReceipts
//
typedef void* BREthereumLESReceiptsContext;

typedef void
(*BREthereumLESReceiptsCallback) (BREthereumLESBlockBodiesContext context,
                                  BREthereumHash block,
                                  BREthereumTransactionReceipt receipts[]);

extern BREthereumLESStatus
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BREthereumHash blocks[]);

extern BREthereumLESStatus
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash block);

//
// Proofs
//
//
typedef void* BREthereumLESProofsV2Context;

typedef void
(*BREthereumLESProofsV2Callback) (BREthereumLESProofsV2Context context,
                                  BREthereumHash blockHash,
                                  BREthereumHash key,
                                  BREthereumHash key2);

extern BREthereumLESStatus
lesGetGetProofsV2One (BREthereumLES les,
                     BREthereumLESProofsV2Context context,
                     BREthereumLESProofsV2Callback callback,
                     BREthereumHash blockHash,
                     BREthereumHash key,
                     BREthereumHash key2,
                     uint64_t fromLevel);
                     
//
// LES GetTxStatus
//
typedef void* BREthereumLESTransactionStatusContext;

typedef void
(*BREthereumLESTransactionStatusCallback) (BREthereumLESTransactionStatusContext context,
                                          BREthereumHash transaction,
                                          BREthereumTransactionStatus status);

extern BREthereumLESStatus
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BREthereumHash transactions[]);

extern BREthereumLESStatus
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction);

extern BREthereumLESStatus
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransactionRLPType type,
                      BREthereumTransaction transaction);


#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_h */
