//
//  BRPeerManager.h
//
//  Created by Aaron Voisine on 9/2/15.
//  Copyright (c) 2015 breadwallet LLC.
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

#ifndef BRPeerManager_h
#define BRPeerManager_h

#include "BRPeer.h"
#include "BRMerkleBlock.h"
#include "BRTransaction.h"
#include "BRWallet.h"
#include <stddef.h>

#define PEER_MAX_CONNECTIONS 3

typedef struct _BRPeerManager BRPeerManager;

// returns a newly allocated BRPeerManager struct that must be freed by calling BRPeerManagerFree()
BRPeerManager *BRPeerManagerNew(BRWallet *wallet, uint32_t earliestKeyTime, BRMerkleBlock *blocks[], size_t blocksCount,
                                const BRPeer peers[], size_t peersCount);

void BRPeerManagerSetCallbacks(BRPeerManager *manager, void *info,
                               void (*syncStarted)(void *info),
                               void (*syncSucceded)(void *info),
                               void (*syncFailed)(void *info, int error),
                               void (*txStatusUpdate)(void *info),
                               void (*txRejected)(void *info, int rescanRecommended),
                               void (*saveBlocks)(void *info, BRMerkleBlock *blocks[], size_t count),
                               void (*savePeers)(void *info, const BRPeer peers[], size_t count),
                               int (*networkIsReachable)(void *info));

// true if currently connected to at least one peer
int BRPeerMangerIsConnected(BRPeerManager *manager);

// connect to bitcoin peer-to-peer network (also call this whenever networkIsReachable() status changes)
void BRPeerManagerConnect(BRPeerManager *manager);

// rescan blockchain for potentially missing transactions
void BRPeerManagerRescan(BRPeerManager *manager);

// current proof-of-work verified best block height
uint32_t BRPeerManagerLastBlockHeight(BRPeerManager *manager);

// the (unverified) best block height reported by connected peers
uint32_t BRPeerManagerEstimatedBlockHeight(BRPeerManager *manager);

// current network sync progress from 0 to 1
double BRPeerManagerSyncProgress(BRPeerManager *manager);

// returns the number of currently connected peers
size_t BRPeerManagerPeerCount(BRPeerManager *manager);

// publishes tx to bitcoin network
void BRPeerManagerPublishTx(BRPeerManager *manager, BRTransaction *tx, void *info,
                            void (*callback)(void *info, int error));

// number of connected peers that have relayed the transaction
size_t BRPeerMangaerRelayCount(BRPeerManager *manager, UInt256 txHash);

// frees memory allocated for manager
void BRPeerManagerFree(BRPeerManager *manager);

#endif // BRPeerManager_h
