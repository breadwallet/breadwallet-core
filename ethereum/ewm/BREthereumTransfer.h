//
//  BREthereumTransfer.h
//  Core
//
//  Created by Ed Gamble on 7/9/18.
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

#ifndef BR_Ethereum_Transfer_H
#define BR_Ethereum_Transfer_H

#include "../base/BREthereumBase.h"
#include "../blockchain/BREthereumNetwork.h"
#include "../blockchain/BREthereumTransaction.h"
#include "../blockchain/BREthereumLog.h"
#include "BREthereumAmount.h"
#include "BREthereumAccount.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumTransferRecord *BREthereumTransfer;

typedef enum {
    // Created: transfer created in local memory
    TRANSFER_STATUS_CREATED,

    // Signed: transfer signed
    TRANSFER_STATUS_SIGNED,

    // Submitted: transfer submitted
    TRANSFER_STATUS_SUBMITTED,

    // Included: transfer is already included in the canonical chain. data contains an
    // RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
    TRANSFER_STATUS_INCLUDED,

    // Error: transfer sending failed. data contains a text error message
    TRANSFER_STATUS_ERRORED

} BREthereumTransferStatusType;

typedef enum {
    FEE_BASIS_NONE,
    FEE_BASIS_GAS
} BREthereumFeeBasisType;

typedef struct {
    BREthereumFeeBasisType type;
    union {
        struct {
            BREthereumGas limit;
            BREthereumGasPrice price;
        } gas;
    } u;
} BREthereumFeeBasis;

static inline BREthereumGas
feeBasisGetGasLimit (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.limit : gasCreate(0));
}

static inline BREthereumGasPrice
feeBasisGetGasPrice (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.price : gasPriceCreate(etherCreateZero()));
}

/**
 * Transfer Create
 */
extern BREthereumTransfer
transferCreate (BREthereumAddress sourceAddress,
                BREthereumAddress targetAddress,
                BREthereumAmount amount,
                BREthereumFeeBasis feeBasis);

extern BREthereumTransfer
transferCreateWithTransaction (OwnershipGiven BREthereumTransaction transaction);

extern BREthereumTransfer
transferCreateWithLog (OwnershipGiven BREthereumLog log,
                       BREthereumToken token);

extern void
transferRelease (BREthereumTransfer transfer);

extern BREthereumAddress
transferGetSourceAddress (BREthereumTransfer transfer);

extern BREthereumAddress
transferGetTargetAddress (BREthereumTransfer transfer);

extern BREthereumAmount
transferGetAmount (BREthereumTransfer transfer);

extern BREthereumFeeBasis
transferGetFeeBasis (BREthereumTransfer transfer);

extern BREthereumGas
transferGetGasEstimate (BREthereumTransfer transfer);

extern void
transferSetGasEstimate (BREthereumTransfer transfer,
                        BREthereumGas gasEstimate);

extern BREthereumTransaction
transferGetOriginatingTransaction (BREthereumTransfer transfer);

extern BREthereumTransaction
transferGetBasisTransaction (BREthereumTransfer transfer);

extern BREthereumLog
transferGetBasisLog (BREthereumTransfer transfer);

extern void
transferSign (BREthereumTransfer transfer,
              BREthereumNetwork network,
              BREthereumAccount account,
              BREthereumAddress address,
              const char *paperKey);

extern void
transferSignWithKey (BREthereumTransfer transfer,
                     BREthereumNetwork network,
                     BREthereumAccount account,
                     BREthereumAddress address,
                     BRKey privateKey);

//
//
//
#if defined TRANSFER_FEES_AND_STUFF
/**
 * Return the maximum fee (in Ether) for transfer (as gasLimit * gasPrice).
 */
extern BREthereumEther
transferGetFeeLimit (BREthereumTransfer transfer, int *overflow);

extern BREthereumGasPrice
transferGetGasPrice (BREthereumTransfer transfer);

extern void
transferSetGasPrice (BREthereumTransfer transfer,
                     BREthereumGasPrice gasPrice);

extern BREthereumGas
transferGetGasLimit (BREthereumTransfer transfer);

extern void
transferSetGasLimit (BREthereumTransfer transfer,
                     BREthereumGas gasLimit);
#endif

// TODO: If not signed? submitted?
extern const BREthereumHash
transferGetHash (BREthereumTransfer transfer);

// TODO: Needed?
extern uint64_t
transferGetNonce (BREthereumTransfer transfer);

extern BREthereumToken
transferGetToken (BREthereumTransfer transfer);

extern BREthereumEther
transferGetFee (BREthereumTransfer transfer, int *overflow);

extern BREthereumComparison
transferCompare (BREthereumTransfer t1,
                 BREthereumTransfer t2);

//
//
//

extern BREthereumBoolean
transferHasStatusType (BREthereumTransfer transfer,
                       BREthereumTransferStatusType type);

extern BREthereumBoolean
transferHasStatusTypeOrTwo (BREthereumTransfer transfer,
                            BREthereumTransferStatusType type1,
                            BREthereumTransferStatusType type2);


extern int
transferExtractStatusIncluded (BREthereumTransfer transfer,
                               BREthereumGas *gasUsed,
                               BREthereumHash *blockHash,
                               uint64_t *blockNumber,
                               uint64_t *transactionIndex);
extern int
transferExtractStatusError (BREthereumTransfer transfer,
                            char **reason);

extern void
transfersRelease (OwnershipGiven BRArrayOf(BREthereumTransfer) transfers);
    
#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Transfer_H
