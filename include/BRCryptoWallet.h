//
//  BRCryptoWallet.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoWallet_h
#define BRCryptoWallet_h

#include "BRCryptoStatus.h"
#include "BRCryptoKey.h"
#include "BRCryptoNetwork.h"        // NetworkFee
#include "BRCryptoPayment.h"
#include "BRCryptoFeeBasis.h"
#include "BRCryptoTransfer.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// MARK: Forward Declarations

    typedef struct BRCryptoWalletSweeperRecord *BRCryptoWalletSweeper;

    /// MARK: Wallet Event

    typedef enum {
        CRYPTO_WALLET_STATE_CREATED,
        CRYPTO_WALLET_STATE_DELETED
    } BRCryptoWalletState;

    typedef enum {
        CRYPTO_WALLET_EVENT_CREATED,
        CRYPTO_WALLET_EVENT_CHANGED,
        CRYPTO_WALLET_EVENT_DELETED,
        
        CRYPTO_WALLET_EVENT_TRANSFER_ADDED,
        CRYPTO_WALLET_EVENT_TRANSFER_CHANGED,
        CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED,
        CRYPTO_WALLET_EVENT_TRANSFER_DELETED,
        
        CRYPTO_WALLET_EVENT_BALANCE_UPDATED,
        CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED,
        
        CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED,
    } BRCryptoWalletEventType;

    extern const char *
    BRCryptoWalletEventTypeString (BRCryptoWalletEventType t);

    typedef struct {
        BRCryptoWalletEventType type;
        union {
            struct {
                BRCryptoWalletState oldState;
                BRCryptoWalletState newState;
            } state;
            
            struct {
                /// Handler must 'give'
                BRCryptoTransfer value;
            } transfer;
            
            struct {
                /// Handler must 'give'
                BRCryptoAmount amount;
            } balanceUpdated;
            
            struct {
                /// Handler must 'give'
                BRCryptoFeeBasis basis;
            } feeBasisUpdated;
            
            struct {
                /// Handler must 'give' basis
                BRCryptoStatus status;
                BRCryptoCookie cookie;
                BRCryptoFeeBasis basis;
            } feeBasisEstimated;
        } u;
    } BRCryptoWalletEvent;

    extern BRCryptoWalletState
    cryptoWalletGetState (BRCryptoWallet wallet);

    /**
     * Returns the wallet's currency
     *
     * @param wallet the wallet
     *
     * @return The currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoWalletGetCurrency (BRCryptoWallet wallet);

    /**
     * Returns the wallet's (default) unit.  Used for *display* of the wallet's balance.
     *
     * @param wallet The wallet
     *
     * @return the unit w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoWalletGetUnit (BRCryptoWallet wallet);

    extern BRCryptoCurrency
    cryptoWalletGetCurrencyForFee (BRCryptoWallet wallet);

    /**
     * Returns the wallet's fee unit.
     *
     * @param wallet The wallet
     *
     * @return the fee unit w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoWalletGetUnitForFee (BRCryptoWallet wallet);

    /**
     * Returns the wallets balance
     *
     * @param wallet the wallet
     *
     * @return the balance
     */
    extern BRCryptoAmount
    cryptoWalletGetBalance (BRCryptoWallet wallet);

    extern BRCryptoBoolean
    cryptoWalletHasTransfer (BRCryptoWallet wallet,
                             BRCryptoTransfer transfer);

    /**
     * Returns a newly allocated array of the wallet's transfers.
     *
     * The caller is responsible for deallocating the returned array using
     * free().
     *
     * @param wallet the wallet
     * @param count the number of transfers returned
     *
     * @return An array of transfers w/ an incremented reference count (aka 'taken')
     *         or NULL if there are no transfers in the wallet.
     */
    extern BRCryptoTransfer *
    cryptoWalletGetTransfers (BRCryptoWallet wallet,
                              size_t *count);

    /**
     * Returns a 'new' adddress from `wallet` according to the provided `addressScheme`.  For BTC
     * this is a segwit or a bech32 address.  Note that the returned address is not associated with
     * `wallet` and thus one runs the risk of using a BRCryptoAddress w/ the wrong BRCryptoWallet
     */
    extern BRCryptoAddress
    cryptoWalletGetAddress (BRCryptoWallet wallet,
                            BRCryptoAddressScheme addressScheme);

    extern BRCryptoFeeBasis
    cryptoWalletGetDefaultFeeBasis (BRCryptoWallet wallet);

    extern void
    cryptoWalletSetDefaultFeeBasis (BRCryptoWallet wallet,
                                    BRCryptoFeeBasis feeBasis);

    /**
     * Create a transfer.
     *
     * @param wallet The wallet providing the amount
     * @param target The target address; this must be consistent with the provied wallet's address
     * @param amount the amount to transfer
     * @param estimatedFeeBasis the fees one is willing to
     *
     * @return the transfer or NULL
     */
    extern BRCryptoTransfer
    cryptoWalletCreateTransfer (BRCryptoWallet wallet,
                                BRCryptoAddress target,
                                BRCryptoAmount amount,
                                BRCryptoFeeBasis estimatedFeeBasis);

    extern BRCryptoTransfer
    cryptoWalletCreateTransferForWalletSweep (BRCryptoWallet  wallet,
                                              BRCryptoWalletSweeper sweeper,
                                              BRCryptoFeeBasis estimatedFeeBasis);

    extern BRCryptoTransfer
    cryptoWalletCreateTransferForPaymentProtocolRequest (BRCryptoWallet wallet,
                                                         BRCryptoPaymentProtocolRequest request,
                                                         BRCryptoFeeBasis estimatedFeeBasis);

    extern void
    cryptoWalletAddTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);

    extern void
    cryptoWalletRemTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);

    /**
     * Estimate the wallet's maximum or minimun transfer amount.
     */
    extern BRCryptoAmount
    cryptoWalletEstimateLimit (BRCryptoWallet  wallet,
                               BRCryptoBoolean asMaximum,
                               BRCryptoAddress target,
                               BRCryptoNetworkFee fee,
                               BRCryptoBoolean *needEstimate,
                               BRCryptoBoolean *isZeroIfInsuffientFunds);

    /**
     * Estimate the fee to transfer `amount` from `wallet` using the `feeBasis`.  Return an amount
     * represented in the wallet's fee currency.
     *
     * @param wallet the wallet
     * @param amount the amount to transfer
     * @param feeBasis the fee basis for the transfer
     *
     * @return the fee
     */
    extern void
    cryptoWalletEstimateFeeBasis (BRCryptoWallet  wallet,
                                  BRCryptoCookie cookie,
                                  BRCryptoAddress target,
                                  BRCryptoAmount  amount,
                                  BRCryptoNetworkFee fee);

    extern void
    cryptoWalletEstimateFeeBasisForWalletSweep (BRCryptoWallet  wallet,
                                                BRCryptoCookie cookie,
                                                BRCryptoWalletSweeper sweeper,
                                                BRCryptoNetworkFee fee);

    extern void
    cryptoWalletEstimateFeeBasisForPaymentProtocolRequest (BRCryptoWallet wallet,
                                                           BRCryptoCookie cookie,
                                                           BRCryptoPaymentProtocolRequest request,
                                                           BRCryptoNetworkFee fee);

    extern BRCryptoFeeBasis
    cryptoWalletCreateFeeBasis (BRCryptoWallet wallet,
                                BRCryptoAmount pricePerCostFactor,
                                double costFactor);

    extern BRCryptoBoolean
    cryptoWalletEqual (BRCryptoWallet w1, BRCryptoWallet w2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoWallet, cryptoWallet);

    /// MARK: Wallet Sweeper

    typedef enum {
        CRYPTO_WALLET_SWEEPER_SUCCESS,
        CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY,
        CRYPTO_WALLET_SWEEPER_INVALID_KEY,
        CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS,
        CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION,
        CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET,
        CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND,
        CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS,
        CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP,
        
        // calling a sweeper function for the wrong type
        CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION,
    } BRCryptoWalletSweeperStatus;

    extern BRCryptoWalletSweeperStatus
    cryptoWalletSweeperValidateSupported (BRCryptoNetwork network,
                                          BRCryptoCurrency currency,
                                          BRCryptoKey key,
                                          BRCryptoWallet wallet);

    extern BRCryptoWalletSweeper
    cryptoWalletSweeperCreateAsBtc (BRCryptoNetwork network,
                                    BRCryptoCurrency currency,
                                    BRCryptoKey key,
                                    BRCryptoAddressScheme scheme);

    extern void
    cryptoWalletSweeperRelease (BRCryptoWalletSweeper sweeper);

    extern BRCryptoWalletSweeperStatus
    cryptoWalletSweeperHandleTransactionAsBTC (BRCryptoWalletSweeper sweeper,
                                               OwnershipKept uint8_t *transaction,
                                               size_t transactionLen);

    extern BRCryptoKey
    cryptoWalletSweeperGetKey (BRCryptoWalletSweeper sweeper);

    extern char *
    cryptoWalletSweeperGetAddress (BRCryptoWalletSweeper sweeper);

    extern BRCryptoAmount
    cryptoWalletSweeperGetBalance (BRCryptoWalletSweeper sweeper);

    extern BRCryptoWalletSweeperStatus
    cryptoWalletSweeperValidate (BRCryptoWalletSweeper sweeper);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWallet_h */
