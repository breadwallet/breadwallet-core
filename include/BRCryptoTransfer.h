//
//  BRCryptoTransfer.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoTransfer_h
#define BRCryptoTransfer_h

#include "BRCryptoHash.h"
#include "BRCryptoAddress.h"
#include "BRCryptoAmount.h"
#include "BRCryptoFeeBasis.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoTransferRecord *BRCryptoTransfer;

    /// MARK: Transfer Submission Result

    typedef enum {
        CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN,
        CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX,
    } BRCryptoTransferSubmitErrorType;

    typedef struct {
        BRCryptoTransferSubmitErrorType type;
        union {
            struct {
                int errnum;
            } posix;
        } u;
    } BRCryptoTransferSubmitError;

    extern BRCryptoTransferSubmitError
    cryptoTransferSubmitErrorUnknown(void);

    extern BRCryptoTransferSubmitError
    cryptoTransferSubmitErrorPosix(int errnum);

    /**
     * Return a descriptive message as to why the error occurred.
     *
     *@return the detailed reason as a string or NULL
     */
    extern char *
    cryptoTransferSubmitErrorGetMessage(BRCryptoTransferSubmitError *e);

    /// MARK: - Transfer State

    typedef enum {
        CRYPTO_TRANSFER_STATE_CREATED,
        CRYPTO_TRANSFER_STATE_SIGNED,
        CRYPTO_TRANSFER_STATE_SUBMITTED,
        CRYPTO_TRANSFER_STATE_INCLUDED,
        CRYPTO_TRANSFER_STATE_ERRORED,
        CRYPTO_TRANSFER_STATE_DELETED,
    } BRCryptoTransferStateType;

    extern const char *
    cryptoTransferStateTypeString (BRCryptoTransferStateType type);

#define CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE     16
    typedef struct {
        BRCryptoTransferStateType type;
        union {
            struct {
                uint64_t blockNumber;
                uint64_t transactionIndex;
                // This is not assuredly the including block's timestamp; it is the transaction's
                // timestamp which varies depending on how the transaction was discovered.
                uint64_t timestamp;
                BRCryptoFeeBasis feeBasis;

                // transfer that have failed can be included too
                BRCryptoBoolean success;
                char error[CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE + 1];
            } included;

            struct {
                BRCryptoTransferSubmitError error;
            } errored;
        } u;
    } BRCryptoTransferState;

    extern BRCryptoTransferState
    cryptoTransferStateInit (BRCryptoTransferStateType type);

    extern BRCryptoTransferState
    cryptoTransferStateIncludedInit (uint64_t blockNumber,
                                     uint64_t transactionIndex,
                                     uint64_t timestamp,
                                     BRCryptoFeeBasis feeBasis,
                                     BRCryptoBoolean success,
                                     const char *error);

    extern BRCryptoTransferState
    cryptoTransferStateErroredInit (BRCryptoTransferSubmitError error);

    extern BRCryptoTransferState
    cryptoTransferStateCopy (BRCryptoTransferState *state);

    extern void
    cryptoTransferStateRelease (BRCryptoTransferState *state);

    /// MARK: - Transfer Event

    typedef enum {
        CRYPTO_TRANSFER_EVENT_CREATED,
        CRYPTO_TRANSFER_EVENT_CHANGED,
        CRYPTO_TRANSFER_EVENT_DELETED,
    } BRCryptoTransferEventType;

    extern const char *
    cryptoTransferEventTypeString (BRCryptoTransferEventType t);

    typedef struct {
        BRCryptoTransferEventType type;
        union {
            struct {
                BRCryptoTransferState old;
                BRCryptoTransferState new;
            } state;
        } u;
    } BRCryptoTransferEvent;

    /// MARK: - Transfer Direction

    typedef enum {
        CRYPTO_TRANSFER_SENT,
        CRYPTO_TRANSFER_RECEIVED,
        CRYPTO_TRANSFER_RECOVERED
    } BRCryptoTransferDirection;

    /// MARK: - Transfer Attribute

    typedef struct BRCryptoTransferAttributeRecord *BRCryptoTransferAttribute;

    extern const char *
    cryptoTransferAttributeGetKey (BRCryptoTransferAttribute attribute);

    extern const char * // nullable
    cryptoTransferAttributeGetValue (BRCryptoTransferAttribute attribute);

    extern void
    cryptoTransferAttributeSetValue (BRCryptoTransferAttribute attribute, const char *value);

    extern BRCryptoBoolean
    cryptoTransferAttributeIsRequired (BRCryptoTransferAttribute attribute);

    extern BRCryptoTransferAttribute
    cryptoTransferAttributeCopy (BRCryptoTransferAttribute attribute);

    private_extern BRCryptoTransferAttribute
    cryptoTransferAttributeCreate (const char *key,
                                   const char *val, // nullable
                                   BRCryptoBoolean isRequired);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoTransferAttribute, cryptoTransferAttribute);

    typedef enum {
        CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_REQUIRED_BUT_NOT_PROVIDED,
        CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_MISMATCHED_TYPE,
        CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_RELATIONSHIP_INCONSISTENCY
    } BRCryptoTransferAttributeValidationError;

    /// MARK: - Transfer

    /**
     * Returns the transfer's source address
     *
     * @param transfer the transfer
     *
     * @return the source address or NULL
     */
    extern BRCryptoAddress
    cryptoTransferGetSourceAddress (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's target address
     *
     * @param transfer the transfer
     *
     * @return the source address or NULL
     */
   extern BRCryptoAddress
    cryptoTransferGetTargetAddress (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's amount
     *
     * @param transfer the transfer
     *
     * @return the amount
     */
    extern BRCryptoAmount
    cryptoTransferGetAmount (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's amount after considering the direction
     *
     * If we received the transfer, the amount will be positive; if we sent the transfer, the
     * amount will be negative; if the transfer is 'self directed', the amount will be zero.
     *
     * @param transfer the transfer
     *
     * @return the amount
     */
    extern BRCryptoAmount
    cryptoTransferGetAmountDirected (BRCryptoTransfer transfer);

    /**
     * Returns the transfers amount after considering the direction and fee
     *
     * @param transfer the transfer
     *
     * @return the signed, net amoount
     */
    extern BRCryptoAmount
    cryptoTransferGetAmountDirectedNet (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's fee.  Note that the `fee` and the `amount` may be in different
     * currencies.
     *
     * @param transfer the transfer
     *
     * @return the fee
     */
//    extern BRCryptoAmount
//    cryptoTransferGetFee (BRCryptoTransfer transfer);

//    extern BRCryptoBoolean
//    cryptoTransferExtractConfirmation (BRCryptoTransfer transfer,
//                                       uint64_t *blockNumber,
//                                       uint64_t *transactionIndex,
//                                       uint64_t *timestamp,
//                                       BRCryptoAmount *fee);

    extern BRCryptoTransferStateType
    cryptoTransferGetStateType (BRCryptoTransfer transfer);

    extern BRCryptoTransferState
    cryptoTransferGetState (BRCryptoTransfer transfer);

    extern BRCryptoBoolean
    cryptoTransferIsSent (BRCryptoTransfer transfer);

    extern BRCryptoTransferDirection
    cryptoTransferGetDirection (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's hash.  This is the unique identifier for this transfer on the
     * associated network's blockchain.
     *
     * @note: Uniqueness is TBD for Ethereum TOKEN transfers
     *
     * @param transfer the transfer
     *
     * @return the transfer's hash
     */
    extern BRCryptoHash
    cryptoTransferGetHash (BRCryptoTransfer transfer);

    extern BRCryptoUnit
    cryptoTransferGetUnitForAmount (BRCryptoTransfer transfer);

    extern BRCryptoUnit
    cryptoTransferGetUnitForFee (BRCryptoTransfer transfer);

    /**
     * Returns the transfer's feeBasis.
     *
     * @param transfer the transfer
     *
     * @return the transfer's feeBasis
     */
    extern BRCryptoFeeBasis
    cryptoTransferGetEstimatedFeeBasis (BRCryptoTransfer transfer);

    extern BRCryptoFeeBasis
    cryptoTransferGetConfirmedFeeBasis (BRCryptoTransfer transfer);

    extern size_t
    cryptoTransferGetAttributeCount (BRCryptoTransfer transfer);

    extern BRCryptoTransferAttribute
    cryptoTransferGetAttributeAt (BRCryptoTransfer transfer,
                                  size_t index);

    extern BRCryptoBoolean
    cryptoTransferEqual (BRCryptoTransfer transfer1, BRCryptoTransfer transfer2);

    /**
     * Compares two transfers.
     *
     * The transfers are ordered according to the following algorithm:
     *   - IF neither transfer is in the INCLUDED state, they are ordered by pointer identity
     *   - ELSE IF one transfer is in the INCLUDED state, it is "lesser than" one that is NOT
     *   - ELSE both are in the INCLUDED state, order by timestamp, block number and transaction
     *     index (in that order), with those values being compared by magnitude
     *
     * In practice, this means that:
     *   - Transfer A (INCLUDED at time 0, block 0, index 0) is lesser than
     *   - Transfer B (INCLUDED at time 0, block 0, index 1) is lesser than
     *   - Transfer C (INCLUDED at time 0, block 1, index 0) is lesser than
     *   - Transfer D (INCLUDED at time 1, block 0, index 0) is lesser than
     *   - Transfer E (CREATED with pointer 0x10000000) is lesser than
     *   - Transfer F (SIGNED  with pointer 0x20000000) is lesser than
     *   - Transfer G (CREATED with pointer 0x30000000) is lesser than
     *   - Transfer H (DELETED with pointer 0x40000000)
     */
    extern BRCryptoComparison
    cryptoTransferCompare (BRCryptoTransfer transfer1, BRCryptoTransfer transfer2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoTransfer, cryptoTransfer);

    extern void
    cryptoTransferExtractBlobAsBTC (BRCryptoTransfer transfer,
                                    uint8_t **bytes,
                                    size_t   *bytesCount,
                                    uint32_t *blockHeight,
                                    uint32_t *timestamp);


#ifdef __cplusplus
}
#endif

#endif /* BRCryptoTransfer_h */
