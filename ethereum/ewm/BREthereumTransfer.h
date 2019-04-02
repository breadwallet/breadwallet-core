//
//  BREthereumTransfer.h
//  Core
//
//  Created by Ed Gamble on 7/9/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Transfer_H
#define BR_Ethereum_Transfer_H

#include "ethereum/base/BREthereumBase.h"
#include "ethereum/blockchain/BREthereumNetwork.h"
#include "ethereum/blockchain/BREthereumTransaction.h"
#include "ethereum/blockchain/BREthereumLog.h"
#include "BREthereumBase.h"
#include "BREthereumAmount.h"
#include "BREthereumAccount.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRANSACTION_NONCE_IS_NOT_ASSIGNED   UINT64_MAX

static inline BREthereumGas
feeBasisGetGasLimit (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.limit : gasCreate(0));
}

static inline BREthereumGasPrice
feeBasisGetGasPrice (BREthereumFeeBasis basis) {
    return (FEE_BASIS_GAS == basis.type ? basis.u.gas.price : gasPriceCreate(etherCreateZero()));
}

typedef enum  {
    TRANSFER_BASIS_TRANSACTION,
    TRANSFER_BASIS_LOG
} BREthereumTransferBasisType;

/**
 * Transfer Create
 */
extern BREthereumTransfer
transferCreate (BREthereumAddress sourceAddress,
                BREthereumAddress targetAddress,
                BREthereumAmount amount,
                BREthereumFeeBasis feeBasis,
                BREthereumTransferBasisType transferBasisType);

/**
 * Create a transfer from a pre-existing transaction which is to be used as the
 * originatingTransaction.  Generally this function is used to cancel/replace a failed
 * transfer.  The `basisType` is provided - either TRANSACTION or LOG - and must be consistent
 * with the provided transaction.
 *
 * Note: presumably the basis could be derived from the transaction.  Later...
 */
extern BREthereumTransfer
transferCreateWithTransactionOriginating (OwnershipGiven BREthereumTransaction transaction,
                                          BREthereumTransferBasisType transferBasisType);

extern BREthereumTransfer
transferCreateWithTransaction (OwnershipGiven BREthereumTransaction transaction);

extern BREthereumTransfer
transferCreateWithLog (OwnershipGiven BREthereumLog log,
                       BREthereumToken token,
                       BRRlpCoder coder);           // For decoding log->data into UInt256

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

// TODO: If not signed? submitted?

/**
 * Return the transfer's unique identifier; however, it is guaranteed to exist if-and-only-if the
 * transfer's basis has been included in the block chain.  Specifically, if the transfer's basis
 * is a Log then until included we don't even have a Log.
 *
 * @param transfer
 *
 * @return The unique hash or HASH_EMPTY_INIT if the transfer is not in the block chain.
 */
extern const BREthereumHash
transferGetIdentifier (BREthereumTransfer transfer);

/**
 * Return the hash of the transfer's originating transaction.  Multiple transfers might share the
 * same originating transaction and thus the same hash.  For example, an ERC20 exchange would have
 * a 'fee transfer' and a 'token transfer' with the same originating transaction.  Another example
 * would be a single transfer that produced N (> 1) logs (albeit this is not supported yet).
 *
 * @param transfer
 *
 * @return the originating transactions hash or HAHS_EMPTY_INIT
 */
extern const BREthereumHash
transferGetOriginatingTransactionHash (BREthereumTransfer transfer);

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


/**
 * Make `transaction` the basis for `transfer`.  If `transfer` is not based on TRANSACTION, then
 * an assertion is raised.  If `transfer` already has a basis, then that transaction is released.
 *
 * @param transfer
 * @param transaction
 */
extern void
transferSetBasisForTransaction (BREthereumTransfer transfer,
                                OwnershipGiven BREthereumTransaction transaction);

/**
 * Make `log` the basis for `transfer`.  If `transfer` is not based on LOG, then an assertion
 * is raised.  If `transfer` already has a basis, then that log is released.
 *
 * @param transfer
 * @param log
 */
extern void
transferSetBasisForLog (BREthereumTransfer transfer,
                        OwnershipGiven BREthereumLog log);

//
//
//
extern BREthereumTransactionStatus
transferGetStatusForBasis (BREthereumTransfer transfer);

extern void
transferSetStatusForBasis (BREthereumTransfer transfer,
                           BREthereumTransactionStatus status);

extern void
transferSetStatus (BREthereumTransfer transfer,
                   BREthereumTransferStatus status);

extern BREthereumTransferStatus
transferGetStatus (BREthereumTransfer transfer);

extern BREthereumBoolean
transferHasStatus (BREthereumTransfer transfer,
                       BREthereumTransferStatus type);

extern BREthereumBoolean
transferHasStatusOrTwo (BREthereumTransfer transfer,
                            BREthereumTransferStatus type1,
                            BREthereumTransferStatus type2);


extern int
transferExtractStatusIncluded (BREthereumTransfer transfer,
                               BREthereumHash *blockHash,
                               uint64_t *blockNumber,
                               uint64_t *transactionIndex,
                               uint64_t *blockTimestamp,
                               BREthereumGas *gasUsed);

extern int
transferExtractStatusError (BREthereumTransfer transfer,
                            char **reason);

extern int
transferExtractStatusErrorType (BREthereumTransfer transfer,
                                BREthereumTransactionErrorType *type);

extern void
transfersRelease (OwnershipGiven BRArrayOf(BREthereumTransfer) transfers);

//
// Transfer Status
//
extern BREthereumBoolean
transferStatusEqual (BREthereumTransferStatus status1,
                     BREthereumTransferStatus status2);

extern BREthereumTransferStatus
transferStatusCreate (BREthereumTransactionStatus status);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Transfer_H
