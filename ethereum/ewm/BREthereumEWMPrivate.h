//
//  BREthereumEWMPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
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

#ifndef BR_Ethereum_EWM_Private_H
#define BR_Ethereum_EWM_Private_H

#include <pthread.h>
#include "BREthereumEWM.h"
#include "../blockchain/BREthereumBlockChain.h"
#include "../les/BREthereumLES.h"
#include "../bcs/BREthereumBCS.h"
#include "../event/BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// EWM Listener
//
typedef struct  {
    BREthereumListenerContext context;
    BREthereumListenerEWMEventHandler ewmEventHandler;
    BREthereumListenerPeerEventHandler peerEventHandler;
    BREthereumListenerWalletEventHandler walletEventHandler;
    BREthereumListenerBlockEventHandler blockEventHandler;
    BREthereumListenerTransactionEventHandler transactionEventHandler;
} BREthereumEWMListener;

//
// EWM
//

#define DEFAULT_LISTENER_CAPACITY 3
#define DEFAULT_WALLET_CAPACITY 10
#define DEFAULT_BLOCK_CAPACITY 100
#define DEFAULT_TRANSACTION_CAPACITY 1000

typedef enum {
    LIGHT_NODE_CREATED,
    LIGHT_NODE_CONNECTING,
    LIGHT_NODE_CONNECTED,
    LIGHT_NODE_DISCONNECTING,
    LIGHT_NODE_DISCONNECTED,
    LIGHT_NODE_ERRORED
} BREthereumEWMState;

/**
 *
 */
struct BREthereumEWMRecord {
    /**
     * The State
     */
    BREthereumEWMState state;
    
    /**
     * The Type of this EWM
     */
    BREthereumType type;
    
    /**
     * The SyncMode of this EWM
     */
    BREthereumSyncMode syncMode;
    
    /**
     * The network
     */
    BREthereumNetwork network;
    
    /**
     * The Client supporting this EWM
     */
    BREthereumClient client;
    
    /**
     * The account
     */
    BREthereumAccount account;
    
    /**
     * The wallets 'managed/handled' by this ewm.  There can be only one wallet holding ETHER;
     * all the other wallets hold TOKENs and only one wallet per TOKEN.
     */
    BREthereumWallet *wallets;  // for now
    BREthereumWallet  walletHoldingEther;
    
    /**
     * The transactions seen/handled by this ewm.  These are used *solely* for the TransactionId
     * interface in EWM.  *All* transactions must be accesses through their wallet.
     */
    BREthereumTransaction *transactions; // BRSet
    
    /**
     * The blocks handled by this ewm.  [This is currently just those handled for transactions
     * (both Ethererum transactions and logs.  It is unlikely that the current block is here.]
     */
    BREthereumBlock *blocks; // BRSet
    
    /**
     * The BCS Interface
     */
    BREthereumBCS bcs;
    
    /**
     * The BlockHeight is the largest block number seen or computed.  [Note: the blockHeight may
     * be computed from a Log event as (log block number + log confirmations).
     */
    uint64_t blockHeight;
    
    /**
     * An identiifer for a LES/JSON_RPC Request
     */
    unsigned int requestId;
    
    /**
     * The listeners
     */
    BREthereumEWMListener *listeners; // BRArray
    
    /**
     * An EventHandler for Listeners.  All callbacks to the Listener interface occur on a
     * separate thread.
     */
    BREventHandler handlerForListener;
    
    /**
     * An EventHandler for Main.  All 'announcements' (via LES (or JSON_RPC) hit here.
     */
    BREventHandler handlerForMain;
    
    /**
     * The Lock ensuring single thread access to EWM state.
     */
    pthread_mutex_t lock;
};

extern BREthereumWalletId
ewmLookupWalletId(BREthereumEWM ewm,
                  BREthereumWallet wallet);

extern BREthereumWallet
ewmLookupWalletByTransaction (BREthereumEWM ewm,
                              BREthereumTransaction transaction);

extern BREthereumWalletId
ewmInsertWallet (BREthereumEWM ewm,
                 BREthereumWallet wallet);

extern BREthereumBlockId
ewmLookupBlockId (BREthereumEWM ewm,
                  BREthereumBlock block);

extern BREthereumBlockId
ewmInsertBlock (BREthereumEWM ewm,
                BREthereumBlock block);

extern BREthereumTransactionId
ewmLookupTransactionId(BREthereumEWM ewm,
                       BREthereumTransaction transaction);

extern BREthereumTransactionId
ewmInsertTransaction (BREthereumEWM ewm,
                      BREthereumTransaction transaction);

// TODO : NO, eliminate
extern void
ewmDeleteTransaction (BREthereumEWM ewm,
                      BREthereumTransactionId tid);

// =============================================================================================
//
// LES(BCS)/JSON_RPC Callbacks
//
extern const BREventType *handlerEventTypes[];
extern const unsigned int handlerEventTypesCount;

extern void
ewmHandleBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp);

extern void
ewmSignalBlockChain (BREthereumEWM ewm,
                     BREthereumHash headBlockHash,
                     uint64_t headBlockNumber,
                     uint64_t headBlockTimestamp);

//
// Signal/Handle Balance
extern void
ewmHandleAccountState (BREthereumEWM ewm,
                       BREthereumAccountState accountState);

extern void
ewmSignalAccountState (BREthereumEWM ewm,
                       BREthereumAccountState accountState);

//
// Signal/Handle Balance
//
extern void
ewmHandleBalance (BREthereumEWM ewm,
                  BREthereumAmount balance);

extern void
ewmSignalBalance (BREthereumEWM ewm,
                  BREthereumAmount balance);

//
// Signal/Handle GasPrice
//
extern void
ewmHandleGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice);
extern void
ewmSignalGasPrice (BREthereumEWM ewm,
                   BREthereumWallet wallet,
                   BREthereumGasPrice gasPrice);

//
// Signal/Handle GasEstimate
//
extern void
ewmHandleGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      BREthereumGas gasEstimate);
extern void
ewmSignalGasEstimate (BREthereumEWM ewm,
                      BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      BREthereumGas gasEstimate);

//
// Signal/Handle Transaction
//
extern void
ewmHandleTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      BREthereumTransaction transaction);

extern void
ewmSignalTransaction (BREthereumEWM ewm,
                      BREthereumBCSCallbackTransactionType type,
                      BREthereumTransaction transaction);

//
// Signal/Handle Log
//
extern void
ewmHandleLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              BREthereumLog log);

extern void
ewmSignalLog (BREthereumEWM ewm,
              BREthereumBCSCallbackLogType type,
              BREthereumLog log);

//
// Save Blocks
//
extern void
ewmHandleSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks);

extern void
ewmSignalSaveBlocks (BREthereumEWM ewm,
                     BRArrayOf(BREthereumBlock) blocks);

//
// Save Peers
//
extern void
ewmHandleSavePeers (BREthereumEWM ewm,
                    BRArrayOf(BREthereumPeerConfig) peers);

extern void
ewmSignalSavePeers (BREthereumEWM ewm,
                    BRArrayOf(BREthereumPeerConfig) peers);


//
// Sync
//
extern void
ewmHandleSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop);

extern void
ewmSignalSync (BREthereumEWM ewm,
               BREthereumBCSCallbackSyncType type,
               uint64_t blockNumberStart,
               uint64_t blockNumberCurrent,
               uint64_t blockNumberStop);

// =============================================================================================
//
// Listener Callbacks
//
extern const BREventType *listenerEventTypes[];
extern const unsigned int listenerEventTypesCount;

//
// Signal/Handle Wallet Event
extern void
ewmListenerHandleWalletEvent(BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription);

extern void
ewmListenerSignalWalletEvent(BREthereumEWM ewm,
                             BREthereumWalletId wid,
                             BREthereumWalletEvent event,
                             BREthereumStatus status,
                             const char *errorDescription);

//
// Signal/Handle Block Event
//
extern void
ewmListenerSignalBlockEvent(BREthereumEWM ewm,
                            BREthereumBlockId bid,
                            BREthereumBlockEvent event,
                            BREthereumStatus status,
                            const char *errorDescription);

extern void
ewmListenerHandleBlockEvent(BREthereumEWM ewm,
                            BREthereumBlockId bid,
                            BREthereumBlockEvent event,
                            BREthereumStatus status,
                            const char *errorDescription);

//
// Signal/Handle Transaction Event
//
extern void
ewmListenerSignalTransactionEvent(BREthereumEWM ewm,
                                  BREthereumWalletId wid,
                                  BREthereumTransactionId tid,
                                  BREthereumTransactionEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription);

extern void
ewmListenerHandleTransactionEvent(BREthereumEWM ewm,
                                  BREthereumWalletId wid,
                                  BREthereumTransactionId tid,
                                  BREthereumTransactionEvent event,
                                  BREthereumStatus status,
                                  const char *errorDescription);

//
// Signal/Handle Peer Event
//
extern void
ewmListenerSignalPeerEvent(BREthereumEWM ewm,
                           // BREthereumWalletId wid,
                           // BREthereumTransactionId tid,
                           BREthereumPeerEvent event,
                           BREthereumStatus status,
                           const char *errorDescription);

extern void
ewmListenerHandlePeerEvent(BREthereumEWM ewm,
                           // BREthereumWalletId wid,
                           // BREthereumTransactionId tid,
                           BREthereumPeerEvent event,
                           BREthereumStatus status,
                           const char *errorDescription);

//
// Signal/Handle EWM Event
//
extern void
ewmListenerSignalEWMEvent(BREthereumEWM ewm,
                          // BREthereumWalletId wid,
                          // BREthereumTransactionId tid,
                          BREthereumEWMEvent event,
                          BREthereumStatus status,
                          const char *errorDescription);

extern void
ewmListenerHandleEWMEvent(BREthereumEWM ewm,
                          // BREthereumWalletId wid,
                          // BREthereumTransactionId tid,
                          BREthereumEWMEvent event,
                          BREthereumStatus status,
                          const char *errorDescription);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_EWM_Private_H
