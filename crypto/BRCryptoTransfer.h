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

//#include "ethereum/base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoTransferRecord *BRCryptoTransfer;

    typedef enum {
        CRYPTO_TRANSFER_STATE_CREATED,
        CRYPTO_TRANSFER_STATE_SIGNED,
        CRYPTO_TRANSFER_STATE_SUBMITTED,
        CRYPTO_TRANSFER_STATE_INCLUDED,
        CRYPTO_TRANSFER_STATE_ERRORRED,
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
                char message[128 + 1];
            } errorred;
        } u;
    } BRCryptoTransferState;

    extern char *
    cryptoTransferStateGetErrorMessage (BRCryptoTransferState state);

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

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoTransfer, cryptoTransfer);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoTransfer_h */
