//
//  BREthereumLESInterface.h
//  Core
//
//  Created by Ed Gamble on 5/23/18.
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

#ifndef BR_Ethereum_LES_Interface_H
#define BR_Ethereum_LES_Interface_H

#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumBlockChain.h"

/*!
 * @header
 *
 * @abstract
 * Candidate LES Interface
 *
 * @discussion
 * ...
 * - Need to think about errors.
 *
 * - Should array arguments be: BRArray, * w/ NULL terminator, * w/ explicit size?
 *
 * - Instead of passing BREthereumHash on requests, perhaps the Ethereum object, like
 *   BREthereumBlock should be passed?  Then the implementation decides what to use
 *   for the LES call?
 */

/*!
 * @typedef BREthereumLES
 *
 * @abstract
 * An instance to handle LES functionalty.
 */
typedef struct BREthereumLESStruct *BREthereumLES;

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
                                  uint64_t headTotalDifficulty);

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
 * @param headerPeriodInMilliseconds
 * The periodicity to check for headers.  Should be something like:
 *   [Ethereum Block Period] / 2 (or 3 or 4 perhaps.
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
           BREthereumLESAnnounceContext announceContext,
           BREthereumLESAnnounceCallback announceCallback,
           BREthereumHash headHash,
           uint64_t headNumber,
           uint64_t headTotalDifficulty,
           BREthereumHash genesisHash);

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
extern void
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
                                     BREthereumHash block, // BREthereumBlockHeader?
                                     BREthereumTransaction transactions[],
                                     BREthereumHash ommers[]);

extern void
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BREthereumHash blocks[]);

extern void
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash block);

// Too much - never likely used.
extern void
lesGetBlockBodiesMany (BREthereumLES les,
                       BREthereumLESBlockBodiesContext context,
                       BREthereumLESBlockBodiesCallback callback,
                       BREthereumHash block,
                       ...);

//
// LES GetReceipts
//
typedef void* BREthereumLESReceiptsContext;

typedef void
(*BREthereumLESReceiptsCallback) (BREthereumLESBlockBodiesContext context,
                                  BREthereumHash block,
                                  BREthereumTransactionReceipt receipts[]);

extern void
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BREthereumHash blocks[]);

extern void
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash block);

//
// Proofs
//

// ... omit ...

//
// LES GetTxStatus
//
typedef void* BREthereumLESTransactionStatusContext;

typedef void
(*BREthereumLESTransactionStatusCallback) (BREthereumLESTransactionStatusContext context,
                                           BREthereumHash transaction,
                                           BREthereumTransactionStatus status);

extern void
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BREthereumHash transactions[]);

extern void
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction);

extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransaction transaction);

#endif /* BR_Ethereum_LES_Interface_H */
