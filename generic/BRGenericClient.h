//
//  BRGenericClient.h
//  BRCore
//
//  Created by Ed Gamble on 10/14/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.


#ifndef BRGenericClient_h
#define BRGenericClient_h

#include "BRGenericBase.h"

typedef void *BRGenericClientContext;

typedef void
(*BRGenericGetBlockNumberCallback) (BRGenericClientContext context,
                                    BRGenericManager manager,
                                    int rid);

extern int
genManagerAnnounceBlockNumber (BRGenericManager manager,
                               int rid,
                               uint64_t blockNumber);


typedef void
(*BRGenericGetTransactionsCallback) (BRGenericClientContext context,
                                     BRGenericManager manager,
                                     const char *address,
                                     uint64_t begBlockNumber,
                                     uint64_t endBlockNumber,
                                     int rid);

typedef void
(*BRGenericGetTransfersCallback) (BRGenericClientContext context,
                                  BRGenericManager manager,
                                  const char *address,
                                  uint64_t begBlockNumber,
                                  uint64_t endBlockNumber,
                                  int rid);

extern int // success - data is valid
genManagerAnnounceTransfer (BRGenericManager manager,
                            int rid,
                            BRGenericTransfer transfer);

extern void
genManagerAnnounceTransferComplete (BRGenericManager manager,
                                    int rid,
                                    int success);

typedef void
(*BRGenericSubmitTransactionCallback) (BRGenericClientContext context,
                                       BRGenericManager manager,
                                       BRGenericWallet wallet,
                                       BRGenericTransfer transfer,
                                       OwnershipKept uint8_t * txBytes,
                                       size_t txSize,
                                       BRGenericHash hash,
                                       int rid);

extern void
genManagerAnnounceSubmit (BRGenericManager manager,
                          int rid,
                          BRGenericTransfer transfer,
                          int error);

// MARK: - Generic Client

typedef struct {
    BRGenericClientContext context;
    BRGenericGetBlockNumberCallback getBlockNumber;
    BRGenericGetTransactionsCallback getTransactions;
    BRGenericGetTransfersCallback getTransfers;
    BRGenericSubmitTransactionCallback submitTransaction;
} BRGenericClient;

#endif /* BRGenericClient_h */
