//
//  BREthereumBCS.c
//  Core
//
//  Created by Ed Gamble on 5/24/18.
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

#include <stdlib.h>
#include "BREthereumBCSPrivate.h"

extern BREthereumBCS
bcsCreate (BREthereumNetwork network,
           BREthereumAccount account,
           BREthereumBlockHeader *headers,
           BREthereumBCSListener listener) {
    BREthereumBCS bcs = (BREthereumBCS) calloc (1, sizeof(struct BREthereumBCSStruct));

    // From headers: extract headhash, headNumber, headTotalDifficulty
    // From network: extract genesisHash

    bcs->account = account;
    bcs->address = addressGetRawAddress(accountGetPrimaryAddress(account));
    bcs->filter = bloomFilterCreateAddress(bcs->address);

    bcs->listener = listener;

    bcs->les = lesCreate(network,
                         (BREthereumLESAnnounceContext) bcs,
                         (BREthereumLESAnnounceCallback) bcsSignalAnnounce,
                         hashCreateEmpty(),
                         0, 0,
                         hashCreateEmpty());

    // Create and then start the eventHandler
    bcs->handler = eventHandlerCreate(bcsEventTypes, bcsEventTypesCount);
    eventHandlerStart(bcs->handler);

    return bcs;
}

extern BREthereumLES
bcsGetLES (BREthereumBCS bcs) {
    return bcs->les;
}

#define BCS_SYNC_BLOCK_NUMBER_COUNT  (100)
#define BCS_TRANSACTION_CHECK_STATUS_SECONDS   (1)

extern void
bcsSync (BREthereumBCS bcs,
         uint64_t blockNumber) {
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader, // bcsSync...
                       blockNumber,
                       1,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

extern void
bcsSendTransaction (BREthereumBCS bcs,
                    BREthereumTransaction transaction) {
    lesSubmitTransaction(bcs->les,
                         (BREthereumLESTransactionStatusContext) bcs,
                         (BREthereumTransactionStatusCallback) NULL,
                         transaction,
                         1000 * BCS_TRANSACTION_CHECK_STATUS_SECONDS);
}

/*!
 * @function bcsHandleAnnounce
 *
 * @abstract
 * Handle a LES 'announce' result.
 */
extern void
bcsHandleAnnounce (BREthereumBCS bcs,
                   BREthereumHash headHash,
                   uint64_t headNumber,
                   uint64_t headTotalDifficulty) {
    // Request the block.
    lesGetBlockHeaders(bcs->les,
                       (BREthereumLESBlockHeadersContext) bcs,
                       (BREthereumLESBlockHeadersCallback) bcsSignalBlockHeader,
                       headNumber,
                       1,
                       0,
                       ETHEREUM_BOOLEAN_FALSE);
}

/*!
 */
extern void
bcsHandleBlockHeader (BREthereumBCS bcs,
                      BREthereumBlockHeader header) {

    // Save the Header
    //   Extend the chain
    //     Or mark as orphan

    // check the bloomFilter for a match
    if (ETHEREUM_BOOLEAN_IS_TRUE (blockHeaderMatch(header, bcs->filter))) {
        // Get the blockbody
        lesGetBlockBodiesOne(bcs->les,
                             (BREthereumLESBlockBodiesContext) bcs,
                             (BREthereumLESBlockBodiesCallback) bcsSignalBlockBodies,
                             blockHeaderGetHash(header));
    }

    // Did the bloom filter match from/to and contract from/to?
    else if (ETHEREUM_BOOLEAN_IS_TRUE(ETHEREUM_BOOLEAN_FALSE)) {

    }

    // Save blocks
}

/*!
 */
extern void
bcsHandleBlockBodies (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transactions[],
                      BREthereumHash ommers[]) {

    // Request the receipts straight away
    lesGetReceiptsOne(bcs->les,
                      (BREthereumLESReceiptsContext) bcs,
                      (BREthereumLESReceiptsCallback) NULL,
                      blockHash);

    for (BREthereumTransaction *txs = transactions; NULL != *txs; txs++) {
        if (ETHEREUM_BOOLEAN_IS_TRUE(transactionHasAddress(*txs, bcs->address))) {
            // it is ours - announce-ish

            // get the account state
            // announce: nonce, balance
        }
    }
}

extern void
bcsHandleTransaction (BREthereumBCS bcs,
                      BREthereumHash blockHash,
                      BREthereumTransaction transaction) {
    // INCLUDED
}

/*!
 */
extern void
bcsHandleTransactionStatus (BREthereumBCS bcs,
                            BREthereumHash transactionHash,
                            BREthereumTransactionStatus status) {
    BREthereumTransaction transaction = NULL;

    //
    // TODO: Do we get 'included' before or after we see transaction, in the block body?
    //  If we get 'included' we should ignore because a) we'll see the transaction
    //  later and b) we don't have any block information to provide in transaction anyway.
    //
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            break;
        case TRANSACTION_STATUS_QUEUED:
            break;
        case TRANSACTION_STATUS_PENDING:
            transactionAnnounceSubmitted(transaction, transactionHash);
            // TRANSACTION_EVENT_SUBMITTED, SUCCESS
            break;

        case TRANSACTION_STATUS_INCLUDED: {
            BREthereumBlockHeader header = NULL; // status.u.included.blockHash
            transactionAnnounceBlocked(transaction,
                                       gasCreate(0),
                                       status.u.included.blockHash,
                                       status.u.included.blockNumber,
                                       status.u.included.transactionIndex);
        }
            break;

        case TRANSACTION_STATUS_ERROR:
            transactionAnnounceDropped(transaction, 0);
            // TRANSACTION_EVENT_SUBMITTED, ERROR_TRANSACTION_SUBMISSION, event->status.u.error.message
            break;

        case TRANSACTION_STATUS_CREATED:
        case TRANSACTION_STATUS_SIGNED:
        case TRANSACTION_STATUS_SUBMITTED:
            // TODO: DO
            break;
    }
    bcs->listener.transactionCallback (bcs->listener.context, transaction);
}

/*!
 */
extern void
bcsHandleTransactionReceipt (BREthereumBCS bcs,
                             BREthereumHash blockHash,
                             BREthereumTransactionReceipt receipt,
                             unsigned int receiptIndex) {
    if (transactionReceiptMatch(receipt, bcs->filter)) {
        BREthereumBlock block = NULL; //blockHash
        BREthereumTransaction transaction = blockGetTransaction(block, receiptIndex);

        size_t logsCount = transactionReceiptGetLogsCount(receipt);
        for (size_t index = 0; index < logsCount; index++) {
            BREthereumLog log = transactionReceiptGetLog(receipt, index);

            // Lookup token...
            //   Confirm 'transfer topic'

            // something related to transaction+log maybe TRANSACTION_EVENT_BLOCKED and then
            // gasCreate (transactionReceiptGetGasUsed(event->receipt))
            // bcs->listener.transactionCallback (bcs->listener.context, bcs, transaction);
        }
    }
}

/*!
 */
extern void
bcsHandleLog (BREthereumBCS bcs,
              BREthereumHash blockHash,
              BREthereumHash transactionHash,
              BREthereumLog log) {
}

