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
    BRCryptoTransferSubmitErrorUnknown(void);

    extern BRCryptoTransferSubmitError
    BRCryptoTransferSubmitErrorPosix(int errnum);

    /**
     * Return a descriptive message as to why the error occurred.
     *
     *@return the detailed reason as a string or NULL
     */
    extern char *
    BRCryptoTransferSubmitErrorGetMessage(BRCryptoTransferSubmitError *e);

    /// MARK: - Transfer State

    typedef enum {
        CRYPTO_TRANSFER_STATE_CREATED,
        CRYPTO_TRANSFER_STATE_SIGNED,
        CRYPTO_TRANSFER_STATE_SUBMITTED,
        CRYPTO_TRANSFER_STATE_INCLUDED,
        CRYPTO_TRANSFER_STATE_ERRORED,
        CRYPTO_TRANSFER_STATE_DELETED,
    } BRCryptoTransferStateType;

    typedef struct {
        BRCryptoTransferStateType type;
        union {
            struct {
                uint64_t blockNumber;
                uint64_t transactionIndex;
                uint64_t timestamp;
                BRCryptoAmount fee;
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
                                     BRCryptoAmount fee);

    extern BRCryptoTransferState
    cryptoTransferStateErroredInit (BRCryptoTransferSubmitError error);

    extern BRCryptoTransferState
    cryptoTransferStateCopy (BRCryptoTransferState *state);

    extern void
    cryptoTransferStateRelease (BRCryptoTransferState *state);

    typedef enum {
        CRYPTO_TRANSFER_EVENT_CREATED,
        CRYPTO_TRANSFER_EVENT_CHANGED,
        CRYPTO_TRANSFER_EVENT_DELETED,
    } BRCryptoTransferEventType;

    extern const char *
    BRCryptoTransferEventTypeString (BRCryptoTransferEventType t);

    typedef struct {
        BRCryptoTransferEventType type;
        union {
            struct {
                BRCryptoTransferState old;
                BRCryptoTransferState new;
            } state;
        } u;
    } BRCryptoTransferEvent;

    typedef enum {
        CRYPTO_TRANSFER_SENT,
        CRYPTO_TRANSFER_RECEIVED,
        CRYPTO_TRANSFER_RECOVERED
    } BRCryptoTransferDirection;


    extern BRCryptoBlockChainType
    cryptoTransferGetType (BRCryptoTransfer transfer);

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
