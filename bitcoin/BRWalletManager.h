//
//  BRWalletManager.h
//  BRCore
//
//  Created by Ed Gamble on 11/21/18.
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

#ifndef BRWalletManager_h
#define BRWalletManager_h

#include <stdio.h>
#include "BRSyncMode.h"
#include "BRBase.h"                 // Ownership
#include "BRBIP32Sequence.h"        // BRMasterPubKey
#include "BRChainParams.h"          // BRChainParams (*NOT THE STATIC DECLARATIONS*)
#include "BRTransaction.h"
#include "BRWallet.h"
#include "BRPeerManager.h"          // Unneeded, if we shadow some functions (connect,disconnect,scan)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRWalletManagerStruct *BRWalletManager;

// Likely unneeded.
typedef enum {
    WALLET_FORKID_BITCOIN = 0x00,
    WALLET_FORKID_BITCASH = 0x40,
    WALLET_FORKID_BITGOLD = 0x4f
} BRWalletForkId;

// Cookies are used as markers to match up an asynchronous operation
// request with its corresponding event.
typedef void *BRCookie;

typedef void *BRWalletManagerClientContext;

/// MARK: - Callbacks

typedef void
(*BRGetBlockNumberCallback) (BRWalletManagerClientContext context,
                             BRWalletManager manager,
                             int rid);

extern int
bwmAnnounceBlockNumber (BRWalletManager manager,
                        int rid,
                        uint64_t blockNumber);

typedef void
(*BRGetTransactionsCallback) (BRWalletManagerClientContext context,
                              BRWalletManager manager,
                              OwnershipKept const char **addresses,
                              size_t addressCount,
                              uint64_t begBlockNumber,
                              uint64_t endBlockNumber,
                              int rid);

extern int // success - data is valid
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        OwnershipKept uint8_t *transaction,
                        size_t transactionLength,
                        uint64_t timestamp,
                        uint64_t blockHeight);

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int id,
                                int success);

typedef void
(*BRSubmitTransactionCallback) (BRWalletManagerClientContext context,
                                BRWalletManager manager,
                                BRWallet *wallet,
                                OwnershipKept uint8_t *transaction,
                                size_t transactionLength,
                                UInt256 transactionHash,
                                int rid);

extern void
bwmAnnounceSubmit (BRWalletManager manager,
                   int rid,
                   UInt256 txHash,
                   int error);

///
/// Transaction Event
///
typedef enum {
    // NOTE: The transaction is NOT owned by the event handler.
    /**
     * For P2P and API, this event occurs once a transaction has been created for a
     * wallet, but is not yet signed or added.
     */
    BITCOIN_TRANSACTION_CREATED,

    /**
     * For P2P and API, this event occurs once a transaction has been signed for a
     * wallet, but is not yet added.
     */
    BITCOIN_TRANSACTION_SIGNED,

    /**
     * For P2P and API, this event occurs once a transaction has been added to a
     * wallet (via BRWalletRegisterTransaction). Transactions are added when they have
     * been relayed by the P2P network or synced via BlockchainDB.
     */
    BITCOIN_TRANSACTION_ADDED,

    /**
     * For P2P, this event occurs once a transaction has been marked as CONFIRMED/UNCONFIRMED
     * by the P2P network.
     *
     * For API, this event does not occur as transactions are implicitly CONFIRMED when synced.
     */
    BITCOIN_TRANSACTION_UPDATED,

    /**
     * For P2P, this event occurs once a transaction has been deleted from a wallet
     * (via BRWalletRemoveTransaction) as a result of an UNCONFIRMED transaction no longer
     * being visible in the mempools of any of connected P2P peers.
     *
     * For API, this event does not occur as transactions are implicitly CONFIRMED when synced.
     */
    BITCOIN_TRANSACTION_DELETED,
} BRTransactionEventType;

typedef struct {
    BRTransactionEventType type;
    union {
        struct {
            uint64_t blockHeight;
            uint32_t timestamp;
        } updated;
    } u;
} BRTransactionEvent;

typedef void
(*BRTransactionEventCallback) (BRWalletManagerClientContext context,
                               OwnershipKept BRWalletManager manager,
                               OwnershipKept BRWallet *wallet,
                               OwnershipKept BRTransaction *transaction,
                               BRTransactionEvent event);

///
/// Wallet Event
///
typedef enum {
    BITCOIN_WALLET_CREATED,
    BITCOIN_WALLET_BALANCE_UPDATED,
    BITCOIN_WALLET_TRANSACTION_SUBMITTED,
    BITCOIN_WALLET_FEE_PER_KB_UPDATED,
    BITCOIN_WALLET_FEE_ESTIMATED,
    BITCOIN_WALLET_DELETED,
} BRWalletEventType;

typedef struct {
    BRWalletEventType type;
    union {
        struct {
            uint64_t satoshi;
        } balance;

        struct {
            BRTransaction *transaction;
            int error; // 0 on success
        } submitted;

        struct {
            uint64_t value;
        } feePerKb;

        struct {
            BRCookie cookie;
            uint64_t feePerKb;
            uint32_t sizeInByte;
        } feeEstimated;
    } u;
} BRWalletEvent;

typedef void
(*BRWalletEventCallback) (BRWalletManagerClientContext context,
                          OwnershipKept BRWalletManager manager,
                          OwnershipKept BRWallet *wallet,
                          BRWalletEvent event);

///
/// WalletManager Event
///
typedef enum {
    BITCOIN_WALLET_MANAGER_CREATED,
    BITCOIN_WALLET_MANAGER_CONNECTED,
    BITCOIN_WALLET_MANAGER_DISCONNECTED,
    BITCOIN_WALLET_MANAGER_SYNC_STARTED,
    BITCOIN_WALLET_MANAGER_SYNC_PROGRESS,
    BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
    BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED
} BRWalletManagerEventType;

typedef struct {
    BRWalletManagerEventType type;
    union {
        struct {
            uint32_t percentComplete;
        } syncProgress;
        struct {
            int error;
        } syncStopped;
        struct {
            uint64_t value;
        } blockHeightUpdated;
    } u;
} BRWalletManagerEvent;

typedef void
(*BRWalletManagerEventCallback) (BRWalletManagerClientContext context,
                                 OwnershipKept BRWalletManager manager,
                                 BRWalletManagerEvent event);

typedef struct {
    BRWalletManagerClientContext context;

    BRGetBlockNumberCallback  funcGetBlockNumber;
    BRGetTransactionsCallback funcGetTransactions;
    BRSubmitTransactionCallback funcSubmitTransaction;

    BRTransactionEventCallback funcTransactionEvent;
    BRWalletEventCallback  funcWalletEvent;
    BRWalletManagerEventCallback funcWalletManagerEvent;
} BRWalletManagerClient;

extern BRWalletManager
BRWalletManagerNew (BRWalletManagerClient client,
                    BRMasterPubKey mpk,
                    const BRChainParams *params,
                    uint32_t earliestKeyTime,
                    BRSyncMode mode,
                    const char *storagePath,
                    uint64_t blockHeight);

extern void
BRWalletManagerFree (BRWalletManager manager);

extern void
BRWalletManagerStart (BRWalletManager manager);

extern void
BRWalletManagerStop (BRWalletManager manager);

extern void
BRWalletManagerConnect (BRWalletManager manager);

extern void
BRWalletManagerDisconnect (BRWalletManager manager);

extern void
BRWalletManagerScan (BRWalletManager manager);

//
// These should not be needed if the events are sufficient
//
extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager);

/**
 * Creates an unsigned transaction that sends the specified amount from the wallet to the given address.
 *
 * @returns NULL on failure, or a transaction on success; the returned transaction must be freed using BRTransactionFree()
 */
extern BRTransaction *
BRWalletManagerCreateTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  uint64_t amount,
                                  const char *addr,
                                  uint64_t feePerKb);

/**
 * Signs any inputs in transaction that can be signed using private keys from the wallet.
 *
 * @seed the master private key (wallet seed) corresponding to the master public key given when the wallet was created
 *
 * @return true if all inputs were signed, or false if there was an error or not all inputs were able to be signed
 */
extern int
BRWalletManagerSignTransaction (BRWalletManager manager,
                                BRWallet *wallet,
                                OwnershipKept BRTransaction *transaction,
                                const void *seed,
                                size_t seedLen);

extern void
BRWalletManagerSubmitTransaction (BRWalletManager manager,
                                  BRWallet *wallet,
                                  OwnershipKept BRTransaction *transaction);

extern void
BRWalletManagerUpdateFeePerKB (BRWalletManager manager,
                               BRWallet *wallet,
                               uint64_t feePerKb);

extern void
BRWalletManagerEstimateFeeForTransfer (BRWalletManager manager,
                                       BRWallet *wallet,
                                       BRCookie cookie,
                                       uint64_t transferAmount,
                                       uint64_t feePerKb);

#ifdef __cplusplus
}
#endif


#endif /* BRWalletManager_h */
