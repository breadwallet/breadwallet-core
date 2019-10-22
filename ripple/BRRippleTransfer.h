//
//  BRRippleTransfer.h
//  Core
//
//  Created by Carl Cherry on 2019-09-05
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_transfer_h
#define BRRipple_transfer_h

#include "BRRippleBase.h"
#include "BRRippleTransaction.h"
#include "BRRippleAddress.h"

typedef struct BRRippleTransferRecord *BRRippleTransfer;

// Create a new transfer for submitting
extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreateNew(BRRippleAddress from, BRRippleAddress to, BRRippleUnitDrops amount);

extern BRRippleTransfer /* caller must free - rippleTransferFree */
rippleTransferCreate(BRRippleAddress from, BRRippleAddress to,
                    BRRippleUnitDrops amount, // For now assume XRP drops.
                    BRRippleTransactionHash hash,
                    uint64_t timestamp, uint64_t blockHeight);

extern void rippleTransferFree(BRRippleTransfer transfer);

// Getters for all the values
extern BRRippleTransactionHash rippleTransferGetTransactionId(BRRippleTransfer transfer);
extern BRRippleUnitDrops rippleTransferGetFee(BRRippleTransfer transfer);
extern BRRippleUnitDrops rippleTransferGetAmount(BRRippleTransfer transfer);
extern BRRippleUnitDrops rippleTransferGetFee(BRRippleTransfer transfer);
extern BRRippleAddress rippleTransferGetSource(BRRippleTransfer transfer);
extern BRRippleAddress rippleTransferGetTarget(BRRippleTransfer transfer);

extern BRRippleTransaction rippleTransferGetTransaction(BRRippleTransfer transfer);

#endif
