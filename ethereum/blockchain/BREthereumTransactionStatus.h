//
//  BREthereumTransactionStatus.h
//  BRCore
//
//  Created by Ed Gamble on 5/15/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Transaction_Status_h
#define BR_Ethereum_Transaction_Status_h

#include "ethereum/base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Transaction Status Type is an enumeration of the results of transaction
 * submission to the Ethereum P2P network.  [In practice, these may only be specified in
 * the Geth LESv{1,2} specification; not more generally in the Ethereum specifcation}.
 */
typedef enum {
    /**
     * Unknown (0): transaction is unknown
     */
    TRANSACTION_STATUS_UNKNOWN = 0,

    /**
     *Queued (1): transaction is queued (not processable yet)
     */
    TRANSACTION_STATUS_QUEUED = 1,

    /**
     * Pending (2): transaction is pending (processable)
     */
    TRANSACTION_STATUS_PENDING = 2,

    /**
     * Included (3): transaction is already included in the canonical chain. data contains an
     * RLP-encoded [blockHash: B_32, blockNumber: P, txIndex: P] structure.
     */
    TRANSACTION_STATUS_INCLUDED = 3,

    /**
     * Error (4): transaction sending failed. data contains a text error message
     */
    TRANSACTION_STATUS_ERRORED = 4,
} BREthereumTransactionStatusType;

/**
 * Extracted from observation of Geth error reports, from Geth source code, and from Parity
 * source code (see below for Geth+Parity declaration).  Currently Parity, in PIPv1, provides
 * no transactions error status (https://github.com/paritytech/parity-ethereum/issues/9817
 */
typedef enum {
    TRANSACTION_ERROR_INVALID_SIGNATURE = 0,
    TRANSACTION_ERROR_NONCE_TOO_LOW,
    TRANSACTION_ERROR_BALANCE_TOO_LOW,
    TRANSACTION_ERROR_GAS_PRICE_TOO_LOW,
    TRANSACTION_ERROR_GAS_TOO_LOW,
    TRANSACTION_ERROR_REPLACEMENT_UNDER_PRICED,
    TRANSACTION_ERROR_DROPPED,
    TRANSACTION_ERROR_ALREADY_KNOWN,   // Geth: submit arrives after shared (from other peer)
    TRANSACTION_ERROR_UNKNOWN,
} BREthereumTransactionErrorType;

extern const char *
transactionGetErrorName (BREthereumTransactionErrorType type);

/** In `Status` we'll include a 'reason' string; limit the string to filling out the union. */
#define TRANSACTION_STATUS_DETAIL_BYTES   \
    (sizeof (BREthereumGas) + sizeof (BREthereumHash) + 3 * sizeof(uint64_t) - sizeof (BREthereumTransactionErrorType))

/**
 * An Ethereum Transaction Status is the status of a transaction submitted to the Ethereum
 * P2P network.  It consists of the type and then data specific to the type.  For example, if
 * the type is 'included' then {blockHash, blockNumber, blockTransactionIndex, blockTimestamp,
 * gasUsed} is part of the status.
 */
typedef struct BREthereumTransactionStatusLESRecord {
    BREthereumTransactionStatusType type;
    union {
        struct {
            BREthereumHash blockHash;
            uint64_t blockNumber;
            uint64_t transactionIndex;
            uint64_t blockTimestamp;
            BREthereumGas gasUsed;      // Internal
        } included;

        struct {
            BREthereumTransactionErrorType type;
            char detail[TRANSACTION_STATUS_DETAIL_BYTES + 1];
        } errored;
    } u;
} BREthereumTransactionStatus;

#define TRANSACTION_STATUS_BLOCK_TIMESTAMP_UNKNOWN      (0)

extern BREthereumTransactionStatus
transactionStatusCreate (BREthereumTransactionStatusType type);

extern BREthereumTransactionStatus
transactionStatusCreateIncluded (BREthereumHash blockHash,
                                 uint64_t blockNumber,
                                 uint64_t transactionIndex,
                                 uint64_t blockTimestamp,
                                 BREthereumGas gasUsed);

extern BREthereumTransactionStatus
transactionStatusCreateErrored (BREthereumTransactionErrorType type,
                                const char *detail);

static inline BREthereumBoolean
transactionStatusHasType (const BREthereumTransactionStatus *status,
                          BREthereumTransactionStatusType type) {
    return AS_ETHEREUM_BOOLEAN(status->type == type);
}

extern int
transactionStatusExtractIncluded(const BREthereumTransactionStatus *status,
                                 BREthereumHash *blockHash,
                                 uint64_t *blockNumber,
                                 uint64_t *blockTransactionIndex,
                                 uint64_t *blockTimestamp,
                                 BREthereumGas *gas);

extern BREthereumBoolean
transactionStatusEqual (BREthereumTransactionStatus ts1,
                        BREthereumTransactionStatus ts2);

extern BREthereumTransactionStatus
transactionStatusRLPDecode (BRRlpItem item,
                            const char *reasons[],
                            BRRlpCoder coder);

extern BRRlpItem
transactionStatusRLPEncode (BREthereumTransactionStatus status,
                            BRRlpCoder coder);

extern BRArrayOf (BREthereumTransactionStatus)
transactionStatusDecodeList (BRRlpItem item,
                             const char *reasons[],
                             BRRlpCoder coder);

/* Quasi-Statis - used when the BRD endpoint returns an error */
extern BREthereumTransactionErrorType
lookupTransactionErrorType (const char *reasons[],
                            const char *reason);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Transaction_Status_h */


// Geth
//var (
//     // ErrInvalidSender is returned if the transaction contains an invalid signature.
//**     ErrInvalidSender = errors.New("invalid sender")
//
//     // ErrNonceTooLow is returned if the nonce of a transaction is lower than the
//     // one present in the local chain.
// **    ErrNonceTooLow = errors.New("nonce too low")
//
//     // ErrUnderpriced is returned if a transaction's gas price is below the minimum
//     // configured for the transaction pool.
//**     ErrUnderpriced = errors.New("transaction underpriced")
//
//     // ErrReplaceUnderpriced is returned if a transaction is attempted to be replaced
//     // with a different one without the required price bump.
//**     ErrReplaceUnderpriced = errors.New("replacement transaction underpriced")
//
//     // ErrInsufficientFunds is returned if the total cost of executing a transaction
//     // is higher than the balance of the user's account.
// **    ErrInsufficientFunds = errors.New("insufficient funds for gas * price + value")
//
//     // ErrIntrinsicGas is returned if the transaction is specified to use less gas
//     // than required to start the invocation.
//**     ErrIntrinsicGas = errors.New("intrinsic gas too low")
//
//     // ErrGasLimit is returned if a transaction's requested gas limit exceeds the
//     // maximum allowance of the current block.
//     ErrGasLimit = errors.New("exceeds block gas limit")
//
//     // ErrNegativeValue is a sanity error to ensure noone is able to specify a
//     // transaction with a negative value.
//     ErrNegativeValue = errors.New("negative value")
//
//     // ErrOversizedData is returned if the input data of a transaction is greater
//     // than some meaningful limit a user might use. This is not a consensus error
//     // making the transaction invalid, rather a DOS protection.
//     ErrOversizedData = [errors.New("oversized data")
// }

// Parity
//pub enum Error {
//    /// Transaction is already imported to the queue
//    AlreadyImported,
//    /// Transaction is not valid anymore (state already has higher nonce)
//**    Old,
//    /// Transaction has too low fee
//    /// (there is already a transaction with the same sender-nonce but higher gas price)
//    TooCheapToReplace,
//    /// Transaction was not imported to the queue because limit has been reached.
//    LimitReached,
//    /// Transaction's gas price is below threshold.
//**    InsufficientGasPrice {
//        /// Minimal expected gas price
//    minimal: U256,
//        /// Transaction gas price
//    got: U256,
//    },
//    /// Transaction's gas is below currently set minimal gas requirement.
//    InsufficientGas {
//        /// Minimal expected gas
//    minimal: U256,
//        /// Transaction gas
//    got: U256,
//    },
//    /// Sender doesn't have enough funds to pay for this transaction
// **   InsufficientBalance {
//        /// Senders balance
//    balance: U256,
//        /// Transaction cost
//    cost: U256,
//    },
//    /// Transactions gas is higher then current gas limit
//    GasLimitExceeded {
//        /// Current gas limit
//    limit: U256,
//        /// Declared transaction gas
//    got: U256,
//    },
//    /// Transaction's gas limit (aka gas) is invalid.
//    InvalidGasLimit(OutOfBounds<U256>),
//    /// Transaction sender is banned.
//    SenderBanned,
//    /// Transaction receipient is banned.
//    RecipientBanned,
//    /// Contract creation code is banned.
//    CodeBanned,
//    /// Invalid chain ID given.
//    InvalidChainId,
//    /// Not enough permissions given by permission contract.
//    NotAllowed,
//    /// Signature error
//**    InvalidSignature(String),
//    /// Transaction too big
//    TooBig,
//    /// Invalid RLP encoding
//    InvalidRlp(String),
//}
//impl fmt::Display for Error {
//    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
//        use self::Error::*;
//        let msg = match *self {
//            AlreadyImported => "Already imported".into(),
//            Old => "No longer valid".into(),
//            TooCheapToReplace => "Gas price too low to replace".into(),
//            LimitReached => "Transaction limit reached".into(),
//            InsufficientGasPrice { minimal, got } =>
//            format!("Insufficient gas price. Min={}, Given={}", minimal, got),
//            InsufficientGas { minimal, got } =>
//            format!("Insufficient gas. Min={}, Given={}", minimal, got),
//            InsufficientBalance { balance, cost } =>
//            format!("Insufficient balance for transaction. Balance={}, Cost={}",
//                    balance, cost),
//            GasLimitExceeded { limit, got } =>
//            format!("Gas limit exceeded. Limit={}, Given={}", limit, got),
//            InvalidGasLimit(ref err) => format!("Invalid gas limit. {}", err),
//            SenderBanned => "Sender is temporarily banned.".into(),
//            RecipientBanned => "Recipient is temporarily banned.".into(),
//            CodeBanned => "Contract code is temporarily banned.".into(),
//            InvalidChainId => "Transaction of this chain ID is not allowed on this chain.".into(),
//            InvalidSignature(ref err) => format!("Transaction has invalid signature: {}.", err),
//            NotAllowed => "Sender does not have permissions to execute this type of transction".into(),
//            TooBig => "Transaction too big".into(),
//            InvalidRlp(ref err) => format!("Transaction has invalid RLP structure: {}.", err),
//        };
//
//        f.write_fmt(format_args!("Transaction error ({})", msg))
//    }
//}
