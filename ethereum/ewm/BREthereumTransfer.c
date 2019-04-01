//
//  BREthereumTransfer.c
//  Core
//
//  Created by Ed Gamble on 7/9/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <string.h>
#include <assert.h>
#include "ethereum/blockchain/BREthereumTransaction.h"
#include "ethereum/blockchain/BREthereumLog.h"
#include "BREthereumTransfer.h"

static void
transferProvideOriginatingTransaction (BREthereumTransfer transfer);

/// MARK: - Status

//#define TRANSFER_STATUS_DETAIL_BYTES   \
//(sizeof (BREthereumGas) + sizeof (BREthereumHash) + 2 * sizeof(uint64_t))
//
//typedef struct BREthereumTransferStatusRecord {
//    BREthereumTransferStatusType type;
//    union {
//        struct {
//            BREthereumGas gasUsed;      // Internal
//            BREthereumHash blockHash;
//            uint64_t blockNumber;
//            uint64_t transactionIndex;
//        } included;
//
//        struct {
//            BREthereumTransactionErrorType type;
//            char detail[TRANSFER_STATUS_DETAIL_BYTES + 1];
//        } errored;
//    } u;
//} BREthereumTransferStatus;

extern BREthereumTransferStatus
transferStatusCreate (BREthereumTransactionStatus status) {
    switch (status.type) {
        case TRANSACTION_STATUS_UNKNOWN:
            return TRANSFER_STATUS_CREATED;

        case TRANSACTION_STATUS_QUEUED:
        case TRANSACTION_STATUS_PENDING:
            return TRANSFER_STATUS_SUBMITTED;

        case TRANSACTION_STATUS_INCLUDED:
            return TRANSFER_STATUS_INCLUDED;

        case TRANSACTION_STATUS_ERRORED:
            return TRANSFER_STATUS_ERRORED;

        default:
            return TRANSFER_STATUS_CREATED;
    }
}

/// MARK: - Basis

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
            transactionRelease (basis->u.transaction);
            basis->u.transaction = NULL;
            break;

        case TRANSFER_BASIS_LOG:
            logRelease (basis->u.log);
            basis->u.log = NULL;
            break;
    }
}

static BREthereumHash
transferBasisGetHash (BREthereumTransferBasis *basis) {
    switch (basis->type) {
        case TRANSFER_BASIS_TRANSACTION: {
            if (NULL == basis->u.transaction) return EMPTY_HASH_INIT;

            return transactionGetHash (basis->u.transaction);
        }

        case TRANSFER_BASIS_LOG: {
            if (NULL == basis->u.log) return EMPTY_HASH_INIT;

            BREthereumHash hash = EMPTY_HASH_INIT;
            logExtractIdentifier(basis->u.log, &hash, NULL);
            return hash;
        }
    }
}

//
// Transfer
//
struct BREthereumTransferRecord {

    /**
     * The source
     */
    BREthereumAddress sourceAddress;

    /*
     * The target
     */
    BREthereumAddress targetAddress;

    /**
     * The amount - which includes a 'currency' and will/must be consistent with the
     * wallet holding this transfer
     */
    BREthereumAmount amount;

    /**
     * The feeBasis as a pair of { gasLimit, gasPrice }.
     */
    BREthereumFeeBasis feeBasis;

    /**
     * The gasEstimate represents the expected Ethereum VM computation required to process this
     * transfer.
     */
    BREthereumGas gasEstimate;

    /**
     * The transaction that originated this transfer.  For a transfer representing an ERC20
     * transfer, the originatingTransaction will be the transaction the produces a Log event
     * for the transfer.
     *
     * If a transfer is newly created, the originatingTransfer will exist but will not have been
     * submitted to the Ethereum network.  It won't have an assigned 'nonce' until the transfer
     * is signed.
     *
     * An originatingTransaction will ONLY EXIST if the User created the transaction.  Transactions
     * that are found (in the P2P network or via the BRD endpoint) that target User, will not
     * have an originatingTransaction.
     *
     * For a transfer representing an ERC20 transfer originated by User, the Log transfer will
     * have an originatingTransaction and, once confirmed in the blockchain, the transfer
     * representing the ETH fee WILL NOT have an originating transaction.
     *
     * Note: The originatingTransaction might be idential ('eq?') to the basis transaction. Double
     * Note: No the originatingTransaction should never be eq? to the basis (too much memory
     * management to ensure).
     */
    BREthereumTransaction originatingTransaction;

    /**
     * The basis - either a transaction or a log.  A Basis exists if-and-only-if this transfer
     * has been included in the block chain.  For an ERC20 transfer, the basis will be a Log; for
     * an ETH transfer, the basis will be a transaction.
     *
     * The basis and the originatingTransaction are independent.  That is, one can have a basis
     * w/o an originating transaction, such as when ETH or TOK are *sent* to User; one can have
     * an originating transaction w/o a basis, such as when the User created but has not submitted
     * a transfer or if the submitted originating transaction failed.
     */
    BREthereumTransferBasis basis;

    /**
     * The status
     */
    BREthereumTransferStatus status;
};

/**
 * Create a transfer of `amount` from `source` to `target.  The `originatingTransaction` can be
 * provided but may be NULL.  There will be no `basis` (but this function is static and all
 * public function should provide the basis).
 */
static BREthereumTransfer
transferCreateDetailed (BREthereumAddress sourceAddress,
                        BREthereumAddress targetAddress,
                        BREthereumAmount amount,
                        BREthereumFeeBasis feeBasis,
                        OwnershipGiven BREthereumTransaction originatingTransaction) {
    BREthereumTransfer transfer = calloc (1, sizeof(struct BREthereumTransferRecord));

    transfer->sourceAddress = sourceAddress;
    transfer->targetAddress = targetAddress;
    transfer->amount = amount;
    transfer->feeBasis = feeBasis;
    transfer->gasEstimate = gasCreate(0);
    transfer->originatingTransaction = originatingTransaction;
    transfer->status = TRANSFER_STATUS_CREATED;

    // NOTE: transfer->basis is unassigned; hence this function is 'static'.

    return transfer;
}

extern BREthereumTransfer
transferCreate (BREthereumAddress sourceAddress,
                BREthereumAddress targetAddress,
                BREthereumAmount amount,
                BREthereumFeeBasis feeBasis,
                BREthereumTransferBasisType transferBasisType) {
    BREthereumTransfer transfer = transferCreateDetailed (sourceAddress,
                                                          targetAddress,
                                                          amount,
                                                          feeBasis,
                                                          NULL);

    // Assigns originatingTransaction
    transferProvideOriginatingTransaction(transfer);

    // Basis.  Note: if the transferBasisType is BASIS_TRANSACTION,  then we *could* fill in the
    // .transaction field; however, for 'reasons of symmetry' we won't fill in the .transaction
    // field except in circumstances where we could fill in the .log field.
    //
    // We used to fill in .transaction with a copy of the originating transaction.
    transfer->basis.type = transferBasisType;

    // Former comment on TRANSFER_BASIS_LOG:
    //
    // We cannot possibly know what the log is; knowing would require us to implement
    // the Ethereum virtual machine.  Granted we are only creating ERC20 transfers and
    // thus could compute the log?

    // Status
    transfer->status = transferStatusCreate(transactionGetStatus(transfer->originatingTransaction));
    return transfer;
}

// TODO: Is this `transaction` the basis?  Used for 'cancel' - how to cancel a token transfer?
extern BREthereumTransfer
transferCreateWithTransactionOriginating (OwnershipGiven BREthereumTransaction transaction,
                                          BREthereumTransferBasisType transferBasisType) {
    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_GAS,
        { .gas = {
            transactionGetGasLimit(transaction),
            transactionGetGasPrice(transaction)
        }}
    };

    // Use `transaction` as the `originatingTransaction`; takes ownership
    BREthereumTransfer transfer = transferCreateDetailed (transactionGetSourceAddress(transaction),
                                                          transactionGetTargetAddress(transaction),
                                                          amountCreateEther (transactionGetAmount(transaction)),
                                                          feeBasis,
                                                          transaction);

    // Basis: See comments above in `transferCreate()`

    transfer->basis.type = transferBasisType;

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
    // Basis - the transfer now owns the transaction.
    transfer->basis = (BREthereumTransferBasis) {
        TRANSFER_BASIS_TRANSACTION,
        { .transaction = transaction }
    };

    // Status
    transfer->status = transferStatusCreate(transactionGetStatus(transaction));

    return transfer;
}

extern BREthereumTransfer
transferCreateWithLog (OwnershipGiven BREthereumLog log,
                       BREthereumToken token,
                       BRRlpCoder coder) {
    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_NONE
    };

    assert (3 == logGetTopicsCount(log));

    // TODO: Is this a log of interest?
    // BREthereumAddress contractAddress = logGetAddress(log);

    BREthereumAddress sourceAddress = logTopicAsAddress(logGetTopic(log, 1));
    BREthereumAddress targetAddress = logTopicAsAddress(logGetTopic(log, 2));

    // Only at this point do we know that log->data is a number.
    BRRlpItem  item  = rlpGetItem (coder, logGetDataShared(log));
    UInt256 value = rlpDecodeUInt256(coder, item, 1);
    rlpReleaseItem (coder, item);

    BREthereumAmount  amount = amountCreateToken (createTokenQuantity(token, value));

    // No originating transaction
    BREthereumTransfer transfer = transferCreateDetailed (sourceAddress,
                                                          targetAddress,
                                                          amount,
                                                          feeBasis,
                                                          NULL);
    // Basis - the transfer now owns the log
    transfer->basis = (BREthereumTransferBasis) {
        TRANSFER_BASIS_LOG,
        { .log = log }
    };

    // Status
    transfer->status = transferStatusCreate(logGetStatus(log));

    return transfer;
}

extern void
transferRelease (BREthereumTransfer transfer) {
    transactionRelease (transfer->originatingTransaction);
    transferBasisRelease (&transfer->basis);
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

    // Generally, you'd only set the gas estimate for a transfer that a) you have originated and
    // b) that you haven't submitted.  Perhaps we should constrain setting the estimate to only
    // transfers that you have originated?  On the other hand, if for display purposed you want
    // to set an estimate and then get the estimate to display, then perhaps originating the
    // transfer should not be required.
    if (NULL != transfer->originatingTransaction)
        transactionSetGasEstimate (transfer->originatingTransaction, gasEstimate);
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

/**
 * The transfer's unique identifier, as a hash.  This identifier CAN ONLY EXIST once the transfer
 * has been included  Essentially, a transfer based on a log cannot have a unique identifier until
 * the log has an `indexInBlock`.  If we thought that the log's identifier could be the
 * originating transaction's log; we'd be wrong because in general one transaction can produce
 * multiple logs (although, for ERC20 transfers it is one transaction <==> one log).
 *
 * @param transfer
 *
 * @return a hash, may be EMPTY_HASH_INIT
 */
extern const BREthereumHash
transferGetIdentifier (BREthereumTransfer transfer) {
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_TRANSACTION:
            return (NULL == transfer->basis.u.transaction ? EMPTY_HASH_INIT : transactionGetHash(transfer->basis.u.transaction));
        case TRANSFER_BASIS_LOG:
            return (NULL == transfer->basis.u.log ? EMPTY_HASH_INIT : logGetHash(transfer->basis.u.log));
    }
}

extern const BREthereumHash
transferGetOriginatingTransactionHash (BREthereumTransfer transfer) {
    // If we have an originatingTransaction - becasue we created the transfer - then return its
    // hash.  Otherwise use the transfer's basis to get the hash
    return  (NULL != transfer->originatingTransaction
             ? transactionGetHash (transfer->originatingTransaction)
             : transferBasisGetHash(&transfer->basis));
}

extern uint64_t
transferGetNonce (BREthereumTransfer transfer) {
    return (NULL != transfer->originatingTransaction
            ? transactionGetNonce (transfer->originatingTransaction)
            : (TRANSFER_BASIS_TRANSACTION == transfer->basis.type && NULL != transfer->basis.u.transaction
               ? transactionGetNonce(transfer->basis.u.transaction)
               : TRANSACTION_NONCE_IS_NOT_ASSIGNED));
}

extern BREthereumEther
transferGetFee (BREthereumTransfer transfer, int *overflow) {
    if (NULL != overflow) *overflow = 0;

    // If we have a basis, then the transfer is confirmed; use the actual fee.
    if (TRANSFER_BASIS_LOG == transfer->basis.type && NULL != transfer->basis.u.log)
        return etherCreateZero();

    else if (TRANSFER_BASIS_TRANSACTION == transfer->basis.type && NULL != transfer->basis.u.transaction)
        return transactionGetFee (transfer->basis.u.transaction, overflow);

    else if (NULL != transfer->originatingTransaction)
        return transactionGetFee (transfer->originatingTransaction, overflow);

    else return etherCreateZero();
}

/// MARK: - Basis

extern void
transferSetBasisForTransaction (BREthereumTransfer transfer,
                                OwnershipGiven BREthereumTransaction transaction) {
    // The transfer must already have a TRANSACTION basis.
    assert (TRANSFER_BASIS_TRANSACTION == transfer->basis.type);
    assert (NULL != transaction);

    // Release a pre-existing transaction
    if (transfer->basis.u.transaction != transaction)
        transferBasisRelease (&transfer->basis);

    transfer->basis = (BREthereumTransferBasis) {
        TRANSFER_BASIS_TRANSACTION,
        { .transaction = transaction }
    };

    transfer->status = transferStatusCreate (transactionGetStatus(transaction));
}

extern void
transferSetBasisForLog (BREthereumTransfer transfer,
                        OwnershipGiven BREthereumLog log) {
    assert (TRANSFER_BASIS_LOG == transfer->basis.type);
    assert (NULL != log);

    // Release a pre-existing log
    if (transfer->basis.u.log != log)
        transferBasisRelease (&transfer->basis);

    transfer->basis = (BREthereumTransferBasis) {
        TRANSFER_BASIS_LOG,
        { .log = log }
    };

    transfer->status = transferStatusCreate (logGetStatus(log));
}

/// MARK: - Status

extern BREthereumTransactionStatus
transferGetStatusForBasis (BREthereumTransfer transfer) {
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_TRANSACTION:
            assert (NULL != transfer->basis.u.transaction || NULL != transfer->originatingTransaction);
            return (NULL != transfer->basis.u.transaction
                    ? transactionGetStatus (transfer->basis.u.transaction)
                    : transactionGetStatus (transfer->originatingTransaction));

        case TRANSFER_BASIS_LOG:
            assert (NULL != transfer->basis.u.log || NULL != transfer->originatingTransaction);
            return (NULL != transfer->basis.u.log
                    ? logGetStatus (transfer->basis.u.log)
                    : transactionGetStatus (transfer->originatingTransaction));
    }
}

extern void
transferSetStatusForBasis (BREthereumTransfer transfer,
                           BREthereumTransactionStatus status) {
    transfer->status = transferStatusCreate(status);
}

extern void
transferSetStatus (BREthereumTransfer transfer,
                   BREthereumTransferStatus status) {
    transfer->status = status;
}

extern BREthereumTransferStatus
transferGetStatus (BREthereumTransfer transfer) {
    return transfer->status;
}

extern BREthereumBoolean
transferHasStatus (BREthereumTransfer transfer,
                   BREthereumTransferStatus type) {
    return AS_ETHEREUM_BOOLEAN(transfer->status == type);
}

extern BREthereumBoolean
transferHasStatusOrTwo (BREthereumTransfer transfer,
                        BREthereumTransferStatus type1,
                        BREthereumTransferStatus type2) {
    return AS_ETHEREUM_BOOLEAN(transfer->status == type1 ||
                               transfer->status == type2);
}

extern int
transferExtractStatusIncluded (BREthereumTransfer transfer,
                               BREthereumHash *blockHash,
                               uint64_t *blockNumber,
                               uint64_t *transactionIndex,
                               uint64_t *blockTimestamp,
                               BREthereumGas *gasUsed) {
    if (TRANSFER_STATUS_INCLUDED != transfer->status) return 0;

    BREthereumTransactionStatus status = transferGetStatusForBasis (transfer);
    if (NULL != blockHash) *blockHash = status.u.included.blockHash;
    if (NULL != blockNumber) *blockNumber = status.u.included.blockNumber;
    if (NULL != transactionIndex) *transactionIndex = status.u.included.transactionIndex;
    if (NULL != blockTimestamp) *blockTimestamp = status.u.included.blockTimestamp;
    if (NULL != gasUsed) *gasUsed = status.u.included.gasUsed;

    return 1;
}

extern int
transferExtractStatusError (BREthereumTransfer transfer,
                            char **reason) {
    if (TRANSFER_STATUS_ERRORED != transfer->status) return 0;

    BREthereumTransactionStatus status = transferGetStatusForBasis (transfer);
    if (NULL != reason) *reason = strdup (transactionGetErrorName (status.u.errored.type));
    
    return 1;
}

extern int
transferExtractStatusErrorType (BREthereumTransfer transfer,
                                BREthereumTransactionErrorType *type) {
    if (TRANSFER_STATUS_ERRORED != transfer->status) return 0;

    BREthereumTransactionStatus status = transferGetStatusForBasis (transfer);
    if (NULL != type) *type = status.u.errored.type;

    return 1;
}

/// MARK: - Originating Transaction

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
        transactionRelease (transfer->originatingTransaction);
    
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
transferGetEffectiveAmountInEther(BREthereumTransfer transfer) {
    switch (transfer->basis.type) {
        case TRANSFER_BASIS_LOG:
            return etherCreateZero();
        case TRANSFER_BASIS_TRANSACTION:
            return transactionGetAmount(NULL != transfer->basis.u.transaction
                                        ? transfer->basis.u.transaction
                                        : transfer->originatingTransaction);
    }
}

extern BREthereumComparison
transferCompare (BREthereumTransfer t1,
                 BREthereumTransfer t2) {
    assert (t1->basis.type == t2->basis.type);
    switch (t1->basis.type) {
        case TRANSFER_BASIS_TRANSACTION:
            return transactionCompare (t1->basis.u.transaction, t2->basis.u.transaction);
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

extern BREthereumBoolean
transferStatusEqual (BREthereumTransferStatus status1,
                     BREthereumTransferStatus status2) {
    return AS_ETHEREUM_BOOLEAN (status1 == status2);
}

