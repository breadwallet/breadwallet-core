//
//  BRRippleTransfer.c
//  Core
//
//  Created by Carl Cherry on 2019-09-05
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include "BRRippleTransfer.h"
#include "BRRippleAccount.h"
#include "BRRippleUtils.h"
#include "BRRipplePrivateStructs.h"
#include "BRRippleTransaction.h"
#include "BRRippleFeeBasis.h"
#include <stdlib.h>

const char * fee = "__fee__";

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreate(BRRippleAddress from, BRRippleAddress to,
                     BRRippleUnitDrops amount, // For now assume XRP drops.
                     BRRippleUnitDrops fee,
                     BRRippleTransactionHash hash,
                     uint64_t timestamp, uint64_t blockHeight,
                     int error)
{
    BRRippleTransfer transfer = (BRRippleTransfer) calloc (1, sizeof (struct BRRippleTransferRecord));
    transfer->sourceAddress = rippleAddressClone (from);
    transfer->targetAddress = rippleAddressClone (to);
    transfer->amount = amount;
    transfer->fee = fee;
    transfer->transactionId = hash;
    transfer->timestamp = timestamp;
    transfer->blockHeight = blockHeight;
    transfer->error = error;
    transfer->transaction = NULL;
    return transfer;
}

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreateNew(BRRippleAddress from, BRRippleAddress to,
                     BRRippleUnitDrops amount)
{
    BRRippleTransfer transfer = (BRRippleTransfer) calloc (1, sizeof (struct BRRippleTransferRecord));
    transfer->sourceAddress = rippleAddressClone (from);
    transfer->targetAddress = rippleAddressClone (to);
    transfer->amount = amount;
    BRRippleFeeBasis feeBasis; // NOTE - hard code for DEMO purposes
    feeBasis.pricePerCostFactor = 10;
    feeBasis.costFactor = 1;
    transfer->fee = 10; // Consistent w/ feeBasis
    transfer->transaction = rippleTransactionCreate(from, to, amount, feeBasis);
    return transfer;
}

extern BRRippleTransfer
rippleTransferClone (BRRippleTransfer transfer) {
    BRRippleTransfer clone = (BRRippleTransfer) calloc (1, sizeof (struct BRRippleTransferRecord));
    memcpy (clone, transfer, sizeof (struct BRRippleTransferRecord));

    if (transfer->sourceAddress)
        clone->sourceAddress = rippleAddressClone (transfer->sourceAddress);

    if (transfer->targetAddress)
        clone->targetAddress = rippleAddressClone (transfer->targetAddress);

    if (transfer->transaction)
        clone->transaction = rippleTransactionClone (transfer->transaction);

    return clone;
}

extern void rippleTransferFree(BRRippleTransfer transfer)
{
    assert(transfer);
    if (transfer->sourceAddress) rippleAddressFree (transfer->sourceAddress);
    if (transfer->targetAddress) rippleAddressFree (transfer->targetAddress);
    if (transfer->transaction) {
        rippleTransactionFree(transfer->transaction);
    }
    free(transfer);
}

// Getters for all the values
extern BRRippleTransactionHash rippleTransferGetTransactionId(BRRippleTransfer transfer)
{
    assert(transfer);
    if (transfer->transaction) {
        // If we have an embedded transaction that means that we created a new tx
        // which by now should have been serialized
        return rippleTransactionGetHash(transfer->transaction);
    } else {
        return transfer->transactionId;
    }
}
extern BRRippleUnitDrops rippleTransferGetAmount(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->amount;
}
extern BRRippleAddress rippleTransferGetSource(BRRippleTransfer transfer)
{
    assert(transfer);
    return rippleAddressClone (transfer->sourceAddress);
}
extern BRRippleAddress rippleTransferGetTarget(BRRippleTransfer transfer)
{
    assert(transfer);
    return rippleAddressClone (transfer->targetAddress);
}

extern BRRippleUnitDrops rippleTransferGetFee(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->fee;
#if 0
    // See the note in BRRippleAcount.h with respect to the feeAddressBytes
    // If the "target" address is set to the feeAddressBytes then return the
    // amount on this transfer - otherwise return 0.
    if (1 == rippleAddressIsFeeAddress(transfer->targetAddress)) {
        return transfer->amount;
    } else {
        return (BRRippleUnitDrops)0L;
    }
#endif
}

extern BRRippleFeeBasis rippleTransferGetFeeBasis (BRRippleTransfer transfer) {
    return (BRRippleFeeBasis) {
        transfer->fee,
        1
    };
}

extern BRRippleTransaction rippleTransferGetTransaction(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->transaction;
}

extern int rippleTransferHasSource (BRRippleTransfer transfer,
                                    BRRippleAddress source) {
    return rippleAddressEqual (transfer->sourceAddress, source);
}

extern int
rippleTransferHasError(BRRippleTransfer transfer) {
    return transfer->error;
}
