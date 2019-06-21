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
                              const char **addresses,
                              size_t addressCount,
                              uint64_t begBlockNumber,
                              uint64_t endBlockNumber,
                              int rid);

extern int // success - data is valid
bwmAnnounceTransaction (BRWalletManager manager,
                        int id,
                        BRTransaction *transaction);

extern void
bwmAnnounceTransactionComplete (BRWalletManager manager,
                                int id,
                                int success);

typedef void
(*BRSubmitTransactionCallback) (BRWalletManagerClientContext context,
                                BRWalletManager manager,
                                BRWallet *wallet,
                                BRTransaction *transaction,
                                int rid);

extern void
bwmAnnounceSubmit (BRWalletManager manager,
                   int rid,
                   BRTransaction *transaction,
                   int error);

///
/// Transaction Event
///
typedef enum {
    BITCOIN_TRANSACTION_ADDED,
    BITCOIN_TRANSACTION_UPDATED,
    BITCOIN_TRANSACTION_DELETED,
} BRTransactionEventType;

typedef struct {
    BRTransactionEventType type;
    union {
        struct {
            uint32_t blockHeight;
            uint32_t timestamp;
        } updated;
    } u;
} BRTransactionEvent;

typedef void
(*BRTransactionEventCallback) (BRWalletManagerClientContext context,
                               BRWalletManager manager,
                               BRWallet *wallet,
                               BRTransaction *transaction,
                               BRTransactionEvent event);

///
/// Wallet Event
///
typedef enum {
    BITCOIN_WALLET_CREATED,
    BITCOIN_WALLET_BALANCE_UPDATED,
    BITCOIN_WALLET_TRANSACTION_SUBMITTED,
    BITCOIN_WALLET_FEE_PER_KB_UPDATED,
    BITCOIN_WALLET_DELETED
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
    } u;
} BRWalletEvent;

typedef void
(*BRWalletEventCallback) (BRWalletManagerClientContext context,
                          BRWalletManager manager,
                          BRWallet *wallet,
                          BRWalletEvent event);

///
/// WalletManager Event
///
typedef enum {
    BITCOIN_WALLET_MANAGER_CREATED,
    BITCOIN_WALLET_MANAGER_CONNECTED,
    BITCOIN_WALLET_MANAGER_DISCONNECTED,
    BITCOIN_WALLET_MANAGER_SYNC_STARTED,
    BITCOIN_WALLET_MANAGER_SYNC_STOPPED,
    BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED
} BRWalletManagerEventType;

typedef struct {
    BRWalletManagerEventType type;
    union {
        struct {
            int error;
        } syncStopped;
        struct {
            uint32_t value;
        } blockHeightUpdated;
    } u;
} BRWalletManagerEvent;

typedef void
(*BRWalletManagerEventCallback) (BRWalletManagerClientContext context,
                                 BRWalletManager manager,
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
                    const char *storagePath);

extern void
BRWalletManagerFree (BRWalletManager manager);

extern void
BRWalletManagerConnect (BRWalletManager manager);

extern void
BRWalletManagerDisconnect (BRWalletManager manager);

extern void
BRWalletManagerScan (BRWalletManager manager);

/**
 * Generate, if needed, unsued addresses up to `limit` entries in the wallet.  The
 * addresses are both 'internal' and 'external' ones.
 */
extern void
BRWalletManagerGenerateUnusedAddrs (BRWalletManager manager);

/**
 * Return an array of unused addresses.  This will generate address, if needed, to provide
 * entries.  The addresses are 'internal' and 'external' ones.
 *
 * This is expected to be used to query the BRD BlockChainDB to identify transactions for
 * manager's wallet.
 *
 * Note: The returned array must be freed
 */
extern BRAddress *
BRWalletManagerGetUnusedAddrs (BRWalletManager manager,
                               size_t *addressCount);

/**
 * Return an array of all addresses, used and unused, tracked by the wallet. The addresses
 * are both 'internal' and 'external' ones.
 *
 * This is expected to be used to query the BRD BlockChainDB to identify transactions for
 * manager's wallet.
 *
 * Note: The returned array must be freed
 */
extern BRAddress *
BRWalletManagerGetAllAddrs (BRWalletManager manager,
                            size_t *addressCount);

//
// These should not be needed if the events are sufficient
//
extern BRWallet *
BRWalletManagerGetWallet (BRWalletManager manager);

extern BRPeerManager *
BRWalletManagerGetPeerManager (BRWalletManager manager);

extern void
BRWalletManagerSubmitTransaction (BRWalletManager manager,
                                  BRTransaction *transaction,
                                  const void *seed,
                                  size_t seedLen);

extern void
BRWalletManagerUpdateFeePerKB (BRWalletManager manager,
                               BRWallet *wallet,
                               uint64_t feePerKb);

#ifdef __cplusplus
}
#endif


#endif /* BRWalletManager_h */
