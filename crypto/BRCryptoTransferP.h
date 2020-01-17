//
//  BRCryptoTransferP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoTransferP_h
#define BRCryptoTransferP_h

#include <pthread.h>

#include "BRCryptoTransfer.h"
#include "BRCryptoBaseP.h"

#include "bitcoin/BRWallet.h"
#include "bitcoin/BRTransaction.h"
#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t blockNumber;
    uint64_t transactionIndex;
    uint64_t timestamp;
    BRCryptoAmount fee; // ouch; => cant be a struct
} BRCryptoTransferConfirmation;


struct BRCryptoTransferRecord {
    pthread_mutex_t lock;

    BRCryptoBlockChainType type;
    union {
        struct {
            BRTransaction *tid;
            uint64_t fee;
            uint64_t send;
            uint64_t recv;
        } btc;
        struct {
            BREthereumEWM ewm;
            BREthereumTransfer tid;
            BREthereumAddress accountAddress;
        } eth;
        BRGenericTransfer gen;
    } u;

    BRCryptoAddress sourceAddress;
    BRCryptoAddress targetAddress;
    BRCryptoTransferState state;

    /// The amount's unit.
    BRCryptoUnit unit;

    /// The fee's unit
    BRCryptoUnit unitForFee;

    /// The feeBasis.  We must include this here for at least the case of BTC where the fees
    /// encoded into the BTC-wire-transaction are based on the BRWalletFeePerKB value at the time
    /// that the transaction is created.  Sometime later, when the feeBasis is needed we can't
    /// go to the BTC wallet and expect the FeePerKB to be unchanged.

    /// Actually this can be derived from { btc.fee / txSize(btc.tid), txSize(btc.tid) }
    BRCryptoFeeBasis feeBasisEstimated;

    BRArrayOf(BRCryptoTransferAttribute) attributes;

    BRCryptoRef ref;
};

private_extern BRCryptoBlockChainType
cryptoTransferGetType (BRCryptoTransfer transfer);

private_extern void
cryptoTransferSetState (BRCryptoTransfer transfer,
                        BRCryptoTransferState state);

private_extern BRCryptoTransfer
cryptoTransferCreateAsBTC (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BRWallet *wid,
                           OwnershipKept BRTransaction *tid,
                           BRCryptoBoolean isBTC); // TRUE if BTC; FALSE if BCH

private_extern BRCryptoTransfer
cryptoTransferCreateAsETH (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BREthereumEWM ewm,
                           BREthereumTransfer tid,
                           BRCryptoFeeBasis feeBasisEstimated);

extern BRCryptoTransfer
cryptoTransferCreateAsGEN (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BRGenericTransfer tid);

private_extern void
cryptoTransferSetConfirmedFeeBasis (BRCryptoTransfer transfer,
                                    BRCryptoFeeBasis feeBasisConfirmed);

private_extern BRTransaction *
cryptoTransferAsBTC (BRCryptoTransfer transfer);

private_extern BREthereumTransfer
cryptoTransferAsETH (BRCryptoTransfer transfer);

private_extern BRGenericTransfer
cryptoTransferAsGEN (BRCryptoTransfer transfer);

private_extern BRCryptoBoolean
cryptoTransferHasBTC (BRCryptoTransfer transfer,
                      BRTransaction *btc);

private_extern BRCryptoBoolean
cryptoTransferHasETH (BRCryptoTransfer transfer,
                      BREthereumTransfer eth);

private_extern BRCryptoBoolean
cryptoTransferHasGEN (BRCryptoTransfer transfer,
                      BRGenericTransfer gen);

private_extern void
cryptoTransferSetAttributes (BRCryptoTransfer transfer,
                             OwnershipKept BRArrayOf(BRCryptoTransferAttribute) attributes);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoTransferP_h */
