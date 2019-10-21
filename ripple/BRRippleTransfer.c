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
#include "generic/BRGenericPrivate.h"
#include <stdlib.h>

const char * fee = "__fee__";

extern BRGenericTransfer
xrpTransferAsGEN (BRRippleTransfer transfer) {
    return &transfer->gen;
}

extern BRRippleTransfer
genTransferAsXRP (BRGenericTransfer transfer) {
    assert (0 == strcmp (XRP_CODE, transfer->type));
    return (BRRippleTransfer) transfer;
}

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreate(BRRippleAddress from, BRRippleAddress to,
                     BRRippleUnitDrops amount, // For now assume XRP drops.
                     BRRippleTransactionHash hash,
                     uint64_t timestamp, uint64_t blockHeight)
{
    BRRippleTransfer transfer = (BRRippleTransfer) genTransferAllocAndInit (XRP_CODE, sizeof (struct BRRippleTransferRecord));
    transfer->sourceAddress = from;
    transfer->targetAddress = to;
    transfer->amount = amount;
    transfer->transactionId = hash;
    transfer->timestamp = timestamp;
    transfer->blockHeight = blockHeight;
    transfer->transaction = NULL;
    return transfer;
}

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreateNew(BRRippleAddress from, BRRippleAddress to,
                     BRRippleUnitDrops amount)
{
    BRRippleTransfer transfer = (BRRippleTransfer) genTransferAllocAndInit (XRP_CODE, sizeof (struct BRRippleTransferRecord));
    transfer->sourceAddress = from;
    transfer->targetAddress = to;
    transfer->amount = amount;
    BRRippleFeeBasis feeBasis; // NOTE - hard code for DEMO purposes
    feeBasis.pricePerCostFactor = 10;
    feeBasis.costFactor = 1;
    transfer->transaction = rippleTransactionCreate(from, to, amount, feeBasis);
    return transfer;
}

extern void rippleTransferFree(BRRippleTransfer transfer)
{
    assert(transfer);
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
    return transfer->sourceAddress;
}
extern BRRippleAddress rippleTransferGetTarget(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->targetAddress;
}

extern BRRippleUnitDrops rippleTransferGetFee(BRRippleTransfer transfer)
{
    assert(transfer);
    // See the note in BRRippleAcount.h with respect to the feeAddressBytes
    // If the "target" address is set to the feeAddressBytes then return the
    // amount on this transfer - otherwise return 0.
    if (memcmp(transfer->targetAddress.bytes, feeAddressBytes, sizeof(transfer->targetAddress.bytes)) == 0) {
        return transfer->amount;
    } else {
        return (BRRippleUnitDrops)0L;
    }
}

extern BRRippleTransaction rippleTransferGetTransaction(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->transaction;
}


