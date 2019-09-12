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
#include <stdlib.h>

const char * fee = "__fee__";

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreate(BRRippleAddress from, BRRippleAddress to,
                     BRRippleUnitDrops amount, // For now assume XRP drops.
                     BRRippleTransactionHash hash,
                     uint64_t timestamp, uint64_t blockHeight)
{
    BRRippleTransfer transfer = calloc (1, sizeof (struct BRRippleTransferRecord));
    transfer->sourceAddress = from;
    transfer->targetAddress = to;
    transfer->amount = amount;
    transfer->transactionId = hash;
    transfer->timestamp = timestamp;
    transfer->blockHeight = blockHeight;
    return transfer;
}

extern void rippleTransferFree(BRRippleTransfer transfer)
{
    assert(transfer);
    free(transfer);
}

// Getters for all the values
extern BRRippleTransactionHash rippleTransferGetTransactionId(BRRippleTransfer transfer)
{
    assert(transfer);
    return transfer->transactionId;
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
    if (memcmp(transfer->targetAddress.bytes, fee, strlen(fee)) == 0) {
        return transfer->amount;
    } else {
        return (BRRippleUnitDrops)0L;
    }
}

