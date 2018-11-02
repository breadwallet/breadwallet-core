//
//  BREthereumTransfer.c
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

#include <string.h>
#include <assert.h>
#include "BREthereumTransfer.h"
#include "../blockchain/BREthereumTransaction.h"
#include "../blockchain/BREthereumLog.h"

#define TRANSACTION_NONCE_IS_NOT_ASSIGNED   UINT64_MAX

static void
transferProvideOriginatingTransaction (BREthereumTransfer transfer);

//
// MARK: - Status
//
#define TRANSFER_STATUS_DETAIL_BYTES   \
(sizeof (BREthereumGas) + sizeof (BREthereumHash) + 2 * sizeof(uint64_t))

typedef struct BREthereumTransferStatusRecord {
    BREthereumTransferStatusType type;
    union {
        struct {
            BREthereumGas gasUsed;      // Internal
            BREthereumHash blockHash;
            uint64_t blockNumber;
            uint64_t transactionIndex;
        } included;
        
        struct {
            BREthereumTransactionErrorType type;
            char detail[TRANSFER_STATUS_DETAIL_BYTES + 1];
        } errored;
    } u;
} BREthereumTransferStatus;

extern BREthereumTransferStatus
transferStatusCreate (BREthereumTransactionStatus status) {
    BREthereumTransferStatus result;

    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            result.type = TRANSFER_STATUS_CREATED;
            break;

        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            result.type = TRANSFER_STATUS_SUBMITTED;
            break;

        case TRANSACTION_STATUS_INCLUDED:
            result.type = TRANSFER_STATUS_INCLUDED;
            result.u.included.gasUsed = status.u.included.gasUsed;
            result.u.included.blockHash = status.u.included.blockHash;
            result.u.included.blockNumber = status.u.included.blockNumber;
            result.u.included.transactionIndex = status.u.included.transactionIndex;
            break;

        case TRANSACTION_STATUS_ERRORED:
            result.type = TRANSFER_STATUS_ERRORED;
            result.u.errored.type = status.u.errored.type;
            memset (result.u.errored.detail, 0, TRANSFER_STATUS_DETAIL_BYTES + 1);
            strncpy (result.u.errored.detail, status.u.errored.detail, TRANSFER_STATUS_DETAIL_BYTES);
            break;

        default:
            result.type = TRANSFER_STATUS_CREATED;
            break;
    }
    return result;
}

//
// MARK: Basis
//
typedef enum  {
    TRANSFER_BASIS_TRANSACTION,
    TRANSFER_BASIS_LOG
} BREthereumTransferBasisType;

typedef struct {
    BREthereumTransferBasisType type;
    union {
        BREthereumTransaction transaction;
        BREthereumLog log;
    } u;
} BREthereumTransferBasis;

static void
transferBasisRelease (BREthereumTransferBasis *basis) {
    switch (basis->type) {
        case TRANSFER_BASIS_TRANSACTION:
            if (NULL != basis->u.transaction) {
                transactionRelease (basis->u.transaction);
                basis->u.transaction = NULL;
            }
            break;

        case TRANSFER_BASIS_LOG:
            if (NULL != basis->u.log) {
                logRelease (basis->u.log);
                basis->u.log = NULL;
            }
            break;
    }
}

//
//
//
struct BREthereumTransferRecord {
    BREthereumAddress sourceAddress;
    
    BREthereumAddress targetAddress;
    
    BREthereumAmount amount;
    
    BREthereumFeeBasis feeBasis;
    
    BREthereumGas gasEstimate;
    
    BREthereumTransaction originatingTransaction;
    
    BREthereumTransferBasis basis;
    
    BREthereumTransferStatus status;
};

static BREthereumTransfer
transferCreateDetailed (BREthereumAddress sourceAddress,
                        BREthereumAddress targetAddress,
                        BREthereumAmount amount,
                        BREthereumFeeBasis feeBasis,
                        BREthereumTransaction originatingTransaction) {
    BREthereumTransfer transfer = calloc (1, sizeof(struct BREthereumTransferRecord));

    transfer->sourceAddress = sourceAddress;
    transfer->targetAddress = targetAddress;
    transfer->amount = amount;
    transfer->feeBasis = feeBasis;
    transfer->gasEstimate = gasCreate(0);
    transfer->originatingTransaction = originatingTransaction;

    return transfer;
}

extern BREthereumTransfer
transferCreate (BREthereumAddress sourceAddress,
                BREthereumAddress targetAddress,
                BREthereumAmount amount,
                BREthereumFeeBasis feeBasis) {
    BREthereumTransfer transfer = transferCreateDetailed(sourceAddress,
                                                         targetAddress,
                                                         amount,
                                                         feeBasis,
                                                         NULL);

    transferProvideOriginatingTransaction(transfer);

    // Basis
    transfer->basis.type = TRANSFER_BASIS_TRANSACTION;
    transfer->basis.u.transaction = transfer->originatingTransaction;

    // Status
    transfer->status = transferStatusCreate(transactionGetStatus(transfer->originatingTransaction));
    return transfer;
}

extern BREthereumTransfer
transferCreateWithTransactionOriginating (OwnershipGiven BREthereumTransaction transaction) {
    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_GAS,
        { .gas = {
            transactionGetGasLimit(transaction),
            transactionGetGasPrice(transaction)
        }}
    };

    // No originating transaction
    BREthereumTransfer transfer = transferCreateDetailed (transactionGetSourceAddress(transaction),
                                                          transactionGetTargetAddress(transaction),
                                                          amountCreateEther (transactionGetAmount(transaction)),
                                                          feeBasis,
                                                          transaction);
    // Basis
    transfer->basis.type = TRANSFER_BASIS_TRANSACTION;
    transfer->basis.u.transaction = transaction;

    // Status
    transfer->status = transferStatusCreate(transactionGetStatus(transaction));

    return transfer;
}

extern BREthereumTransfer
transferCreateWithTransaction (OwnershipGiven BREthereumTransaction transaction) {
    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_GAS,
        { .gas = {
            transactionGetGasLimit(transaction),
            transactionGetGasPrice(transaction)
        }}
    };

    // No originating transaction
    BREthereumTransfer transfer = transferCreateDetailed (transactionGetSourceAddress(transaction),
                                                          transactionGetTargetAddress(transaction),
                                                          amountCreateEther (transactionGetAmount(transaction)),
                                                          feeBasis,
                                                          NULL);
    // Basis
    transfer->basis.type = TRANSFER_BASIS_TRANSACTION;
    transfer->basis.u.transaction = transaction;

    // Status
    transfer->status = transferStatusCreate(transactionGetStatus(transaction));

    return transfer;
}

extern BREthereumTransfer
transferCreateWithLog (OwnershipGiven BREthereumLog log,
                       BREthereumToken token) {
    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_NONE
    };

    assert (3 == logGetTopicsCount(log));

    // TODO: Is this a log of interest?
    // BREthereumAddress contractAddress = logGetAddress(log);

    BREthereumAddress sourceAddress = logTopicAsAddress(logGetTopic(log, 1));
    BREthereumAddress targetAddress = logTopicAsAddress(logGetTopic(log, 2));

    BRRlpData amountData = logGetDataShared(log);
    UInt256 value = rlpDataDecodeUInt256(amountData);

    BREthereumAmount  amount = amountCreateToken (createTokenQuantity(token, value));

    // No originating transaction
    BREthereumTransfer transfer = transferCreateDetailed (sourceAddress,
                                                          targetAddress,
                                                          amount,
                                                          feeBasis,
                                                          NULL);
    // Basis
    transfer->basis.type = TRANSFER_BASIS_LOG;
    transfer->basis.u.log = log;

    // Status
    transfer->status = transferStatusCreate(logGetStatus(log));

    return transfer;
}


extern void
transferRelease (BREthereumTransfer transfer) {
    if (NULL != transfer->originatingTransaction)
        transactionRelease(transfer->originatingTransaction);
    transferBasisRelease(&transfer->basis);
    free (transfer);
}

extern BREthereumAddress
transferGetSourceAddress (BREthereumTransfer transfer) {
    return transfer->sourceAddress;
}

extern BREthereumAddress
transferGetTargetAddress (BREthereumTransfer transfer) {
    return transfer->targetAddress;
}

extern BREthereumAmount
transferGetAmount (BREthereumTransfer transfer) {
    return transfer->amount;
}

extern BREthereumToken
transferGetToken (BREthereumTransfer transfer) {
    return (AMOUNT_TOKEN == amountGetType(transfer->amount)
            ? amountGetToken(transfer->amount)
            : NULL);
}

extern BREthereumFeeBasis
transferGetFeeBasis (BREthereumTransfer transfer) {
    return transfer->feeBasis;
}

extern BREthereumGas
transferGetGasEstimate (BREthereumTransfer transfer) {
    return transfer->gasEstimate;
}

extern void
transferSetGasEstimate (BREthereumTransfer transfer,
                        BREthereumGas gasEstimate) {
    transfer->gasEstimate = gasEstimate;
}

extern BREthereumTransaction
transferGetOriginatingTransaction (BREthereumTransfer transfer) {
    return transfer->originatingTransaction;
}

extern BREthereumTransaction
transferGetBasisTransaction (BREthereumTransfer transfer) {
    return (TRANSFER_BASIS_TRANSACTION == transfer->basis.type
            ? transfer->basis.u.transaction
            : NULL);
}

extern BREthereumLog
transferGetBasisLog (BREthereumTransfer transfer) {
    return (TRANSFER_BASIS_LOG == transfer->basis.type
            ? transfer->basis.u.log
            : NULL);
}

extern void
transferSign (BREthereumTransfer transfer,
              BREthereumNetwork network,
              BREthereumAccount account,
              BREthereumAddress address,
              const char *paperKey) {
    
    if (TRANSACTION_NONCE_IS_NOT_ASSIGNED == transactionGetNonce(transfer->originatingTransaction))
        transactionSetNonce (transfer->originatingTransaction,
                             accountGetThenIncrementAddressNonce(account, address));
    
    // RLP Encode the UNSIGNED transfer
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode (transfer->originatingTransaction,
                                           network,
                                           RLP_TYPE_TRANSACTION_UNSIGNED,
                                           coder);
    BRRlpData data = rlpGetDataSharedDontRelease(coder, item);
    
    // Sign the RLP Encoded bytes.
    BREthereumSignature signature = accountSignBytes (account,
                                                      address,
                                                      SIGNATURE_TYPE_RECOVERABLE_VRS_EIP,
                                                      data.bytes,
                                                      data.bytesCount,
                                                      paperKey);
    
    rlpReleaseItem(coder, item);

    // Attach the signature
    transactionSign (transfer->originatingTransaction, signature);
    // Compute the hash
    item = transactionRlpEncode (transfer->originatingTransaction,
                                 network,
                                 RLP_TYPE_TRANSACTION_SIGNED,
                                 coder);
    transactionSetHash (transfer->originatingTransaction,
                        hashCreateFromData (rlpGetDataSharedDontRelease (coder, item)));

    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

extern void
transferSignWithKey (BREthereumTransfer transfer,
                     BREthereumNetwork network,
                     BREthereumAccount account,
                     BREthereumAddress address,
                     BRKey privateKey) {
    
    if (TRANSACTION_NONCE_IS_NOT_ASSIGNED == transactionGetNonce(transfer->originatingTransaction))
        transactionSetNonce (transfer->originatingTransaction,
                             accountGetThenIncrementAddressNonce(account, address));
    
    // RLP Encode the UNSIGNED transfer
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode (transfer->originatingTransaction,
                                           network,
                                           RLP_TYPE_TRANSACTION_UNSIGNED,
                                           coder);
    BRRlpData data = rlpGetDataSharedDontRelease (coder, item);
    
    // Sign the RLP Encoded bytes.
    BREthereumSignature signature = accountSignBytesWithPrivateKey (account,
                                                                    address,
                                                                    SIGNATURE_TYPE_RECOVERABLE_VRS_EIP,
                                                                    data.bytes,
                                                                    data.bytesCount,
                                                                    privateKey);
    
    rlpReleaseItem(coder, item);

    // Attach the signature
    transactionSign(transfer->originatingTransaction, signature);

    // Compute the hash
    item = transactionRlpEncode (transfer->originatingTransaction,
                                 network,
                                 RLP_TYPE_TRANSACTION_SIGNED,
                                 coder);
    transactionSetHash (transfer->originatingTransaction,
                        hashCreateFromData (rlpGetDataSharedDontRelease (coder, item)));

    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

extern const BREthereumHash
transferGetHash (BREthereumTransfer transfer) {
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_TRANSACTION:
            return transactionGetHash(transfer->basis.u.transaction);
        case TRANSFER_BASIS_LOG:
            return logGetHash(transfer->basis.u.log);
    }
}

extern uint64_t
transferGetNonce (BREthereumTransfer transfer) {
    return (NULL != transfer->originatingTransaction
            ? transactionGetNonce(transfer->originatingTransaction)
            : (TRANSFER_BASIS_TRANSACTION == transfer->basis.type && NULL != transfer->basis.u.transaction
               ? transactionGetNonce(transfer->basis.u.transaction)
               : TRANSACTION_NONCE_IS_NOT_ASSIGNED));
}

extern BREthereumEther
transferGetFee (BREthereumTransfer transfer, int *overflow) {
    if (NULL != overflow) *overflow = 0;
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_LOG:
            return etherCreateZero();
        case TRANSFER_BASIS_TRANSACTION:
            return transactionGetFee(transfer->basis.u.transaction, overflow);
    }
}

///
/// MARK: - Status
///
extern void
transferUpdateStatus (BREthereumTransfer transfer,
                      BREthereumTransactionStatus status) {
    switch (transfer->basis.type){
        case TRANSFER_BASIS_TRANSACTION:
            transactionSetStatus (transfer->basis.u.transaction, status);
            break;
        case TRANSFER_BASIS_LOG:
            logSetStatus (transfer->basis.u.log, status);
            break;
    }

    transfer->status = transferStatusCreate(status);
}

extern BREthereumTransferStatusType
transferGetStatusType (BREthereumTransfer transfer) {
    return transfer->status.type;
}

extern BREthereumBoolean
transferHasStatusType (BREthereumTransfer transfer,
                       BREthereumTransferStatusType type) {
    return AS_ETHEREUM_BOOLEAN(transfer->status.type == type);
}

extern BREthereumBoolean
transferHasStatusTypeOrTwo (BREthereumTransfer transfer,
                            BREthereumTransferStatusType type1,
                            BREthereumTransferStatusType type2) {
    return AS_ETHEREUM_BOOLEAN(transfer->status.type == type1 ||
                               transfer->status.type == type2);
}

extern int
transferExtractStatusIncluded (BREthereumTransfer transfer,
                               BREthereumGas *gasUsed,
                               BREthereumHash *blockHash,
                               uint64_t *blockNumber,
                               uint64_t *transactionIndex) {
    if (TRANSFER_STATUS_INCLUDED != transfer->status.type) return 0;

    if (NULL != gasUsed) *gasUsed = transfer->status.u.included.gasUsed;
    if (NULL != blockHash) *blockHash = transfer->status.u.included.blockHash;
    if (NULL != blockNumber) *blockNumber = transfer->status.u.included.blockNumber;
    if (NULL != transactionIndex) *transactionIndex = transfer->status.u.included.transactionIndex;
    
    return 1;
}

extern int
transferExtractStatusError (BREthereumTransfer transfer,
                            char **reason) {
    if (TRANSFER_STATUS_ERRORED != transfer->status.type) return 0;
    
    if (NULL != reason) *reason = strdup (transactionGetErrorName (transfer->status.u.errored.type));
    
    return 1;
}

extern int
transferExtractStatusErrorType (BREthereumTransfer transfer,
                                BREthereumTransactionErrorType *type) {
    if (TRANSFER_STATUS_ERRORED != transfer->status.type) return 0;

    if (NULL != type) *type = transfer->status.u.errored.type;

    return 1;
}

///
/// MARK: - Originating Transaction
///
static char *
transferProvideOriginatingTransactionData (BREthereumTransfer transfer) {
    switch (amountGetType(transfer->amount)) {
        case AMOUNT_ETHER:
            return strdup ("");
        case AMOUNT_TOKEN: {
            UInt256 value = amountGetTokenQuantity(transfer->amount).valueAsInteger;
            
            char address[ADDRESS_ENCODED_CHARS];
            addressFillEncodedString(transfer->targetAddress, 0, address);
            
            // Data is a HEX ENCODED string
            return (char *) contractEncode (contractERC20, functionERC20Transfer,
                                            // Address
                                            (uint8_t *) &address[2], strlen(address) - 2,
                                            // Amount
                                            (uint8_t *) &value, sizeof (UInt256),
                                            NULL);
        }
    }
}

static BREthereumAddress
transferProvideOriginatingTransactionTargetAddress (BREthereumTransfer transfer) {
    switch (amountGetType(transfer->amount)) {
        case AMOUNT_ETHER:
            return transfer->targetAddress;
        case AMOUNT_TOKEN:
            return tokenGetAddressRaw(amountGetToken(transfer->amount));
    }
}

static BREthereumEther
transferProvideOriginatingTransactionAmount (BREthereumTransfer transfer) {
    switch (amountGetType(transfer->amount)) {
        case AMOUNT_ETHER:
            return transfer->amount.u.ether;
        case AMOUNT_TOKEN:
            return etherCreateZero();
    }
}

static void
transferProvideOriginatingTransaction (BREthereumTransfer transfer) {
    if (NULL != transfer->originatingTransaction)
        transactionRelease(transfer->originatingTransaction);
    
    char *data = transferProvideOriginatingTransactionData(transfer);
    
    transfer->originatingTransaction =
    transactionCreate (transfer->sourceAddress,
                       transferProvideOriginatingTransactionTargetAddress (transfer),
                       transferProvideOriginatingTransactionAmount (transfer),
                       feeBasisGetGasPrice(transfer->feeBasis),
                       feeBasisGetGasLimit(transfer->feeBasis),
                       data,
                       TRANSACTION_NONCE_IS_NOT_ASSIGNED);
    free (data);
}


private_extern BREthereumEther
transferGetEffectiveAmountInEther (BREthereumTransfer transfer) {
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_LOG: return etherCreateZero();
        case TRANSFER_BASIS_TRANSACTION: return transactionGetAmount(transfer->basis.u.transaction);
    }
}

extern BREthereumComparison
transferCompare (BREthereumTransfer t1,
                 BREthereumTransfer t2) {
    assert (t1->basis.type == t2->basis.type);
    switch (t1->basis.type) {
        case TRANSFER_BASIS_TRANSACTION:
            return transactionCompare(t1->basis.u.transaction, t2->basis.u.transaction);
        case TRANSFER_BASIS_LOG:
            return logCompare (t1->basis.u.log, t2->basis.u.log);
    }
}


extern void
transfersRelease (OwnershipGiven BRArrayOf(BREthereumTransfer) transfers) {
    if (NULL != transfers) {
        size_t count = array_count (transfers);
        for (size_t index = 0; index < count; index++)
            transferRelease(transfers[index]);
        array_free (transfers);
    }
}

