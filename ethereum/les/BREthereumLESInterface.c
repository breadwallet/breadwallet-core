//
//  BREthereumLESInterface.c
//  Core
//
//  Created by Ed Gamble on 5/23/18.
//  Copyright © 2018 breadwallet. All rights reserved.
//

#include <assert.h>
#include "BREthereumLESInterface.h"

struct BREthereumLESStruct {
    BREthereumNetwork network;

    // Handler for LES 'Announce' messages.
    BREthereumLESAnnounceContext announceContext;
    BREthereumLESAnnounceCallback announceCallback;

    uint64_t requestId;
};

extern BREthereumLES
lesCreate (BREthereumNetwork network,
           BREthereumLESAnnounceContext announceContext,
           BREthereumLESAnnounceCallback announceCallback,
           BREthereumHash headHash,
           uint64_t headNumber,
           uint64_t headTotalDifficulty,
           BREthereumHash genesisHash) {
    BREthereumLES les = (BREthereumLES) malloc (sizeof (struct BREthereumLESStruct));

    les->network = network;

    les->announceContext = announceContext;
    les->announceCallback = announceCallback;

    les->requestId = 0;

    // Connect with 'handshake status' using headHash, headNumber, ...
    return les;
}

//
// LES GetBlockHeader
//
extern void
lesGetBlockHeaders (BREthereumLES les,
                    BREthereumLESBlockHeadersContext context,
                    BREthereumLESBlockHeadersCallback callback,
                    uint64_t blockNumber,
                    size_t maxBlockCount,
                    uint64_t skip,
                    BREthereumBoolean reverse) {
}

//
// LES GetBlockBodies
//
extern void
lesGetBlockBodies (BREthereumLES les,
                   BREthereumLESBlockBodiesContext context,
                   BREthereumLESBlockBodiesCallback callback,
                   BREthereumHash blocks[]) {

}

extern void
lesGetBlockBodiesOne (BREthereumLES les,
                      BREthereumLESBlockBodiesContext context,
                      BREthereumLESBlockBodiesCallback callback,
                      BREthereumHash block) {

}

extern void
lesGetBlockBodiesMany (BREthereumLES les,
                       BREthereumLESBlockBodiesContext context,
                       BREthereumLESBlockBodiesCallback callback,
                       BREthereumHash block,
                       ...) {
}

//
// LES GetReceipts
//
extern void
lesGetReceipts (BREthereumLES les,
                BREthereumLESReceiptsContext context,
                BREthereumLESReceiptsCallback callback,
                BREthereumHash blocks[]) {
}

extern void
lesGetReceiptsOne (BREthereumLES les,
                   BREthereumLESReceiptsContext context,
                   BREthereumLESReceiptsCallback callback,
                   BREthereumHash block) {

}

//
// LET GetTransactionStatus
//
extern void
lesGetTransactionStatus (BREthereumLES les,
                         BREthereumLESTransactionStatusContext context,
                         BREthereumLESTransactionStatusCallback callback,
                         BREthereumHash transactions[]) {

}

extern void
lesGetTransactionStatusOne (BREthereumLES les,
                            BREthereumLESTransactionStatusContext context,
                            BREthereumLESTransactionStatusCallback callback,
                            BREthereumHash transaction) {

}

extern void
lesSubmitTransaction (BREthereumLES les,
                      BREthereumLESTransactionStatusContext context,
                      BREthereumLESTransactionStatusCallback callback,
                      BREthereumTransaction transaction) {
    assert (ETHEREUM_BOOLEAN_IS_TRUE (transactionIsSigned(transaction)));
    BREthereumHash hash = transactionGetHash(transaction);
    BRRlpData data = transactionEncodeRLP(transaction, les->network, TRANSACTION_RLP_SIGNED);

    // Prepare to handle status...
    // Call SendTxV2
}

