//
//  BRPeerManager.c
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

#include "BRPeerManager.h"
#include "BRPeer.h"
#include "BRBloomFilter.h"
#include "BRMerkleBlock.h"
#include "BRWallet.h"

struct BRPeerManagerContext {
    BRWallet *wallet;
    uint32_t earliestKeyTime;
    int connected;
    BRPeer *peers;
    size_t peersCount;
    BRPeer *connectedPeers;
    size_t connectedCount;
    BRPeer *misbehavinPeers;
    size_t misbehavinCount;
    BRPeer downloadPeer;
    uint32_t tweak;
    uint32_t syncStartHeight;
    uint32_t filterUpdateHeight;
    BRBloomFilter *bloomFilter;
    double fpRate;
    int connectFailures;
    uint32_t lastRelayTime;
    BRMerkleBlock *blocks;
    size_t blocksCount;
    BRMerkleBlock *orphans;
    size_t orphansCount;
    BRMerkleBlock *checkpoints;
    size_t checkpointsCount;
    BRMerkleBlock *lastBlock;
    BRMerkleBlock *lastOrphan;
    struct { UInt256 txHash; BRPeer *peers; size_t count; } *txRelays;
    size_t relayCount;
    BRTransaction *publishedTx;
    void (*publishedCallback)(BRPeerManagerError error, void *info);
    void *publishedInfo;
    size_t publishedCount;
    void (*syncStarted)(BRPeerManager *manager, void *info);
    void (*syncSucceded)(BRPeerManager *manager, void *info);
    void (*syncFailed)(BRPeerManager *manager, BRPeerManagerError error, void *info);
    void (*txStatusUpdate)(BRPeerManager *manager, void *info);
    void *callbackInfo;
};

// returns a newly allocated BRPeerManager struct that must be freed by calling BRPeerManagerFree()
BRPeerManager *BRPeerManagerNew(BRWallet *wallet, uint32_t earliestKeyTime, const BRMerkleBlock blocks[],
                                size_t blocksCount, const BRPeer peers[], size_t peersCount)
{
    return NULL;
}

void BRPeerManagerSetCallbacks(BRPeerManager *manager,
                               void (*syncStarted)(BRPeerManager *manager, void *info),
                               void (*syncSucceded)(BRPeerManager *manager, void *info),
                               void (*syncFailed)(BRPeerManager *manager, BRPeerManagerError error, void *info),
                               void (*txStatusUpdate)(BRPeerManager *manager, void *info),
                               void (*saveBlocks)(BRPeerManager *manager, const BRMerkleBlock blocks[], size_t count,
                                                  void *info),
                               void (*savePeers)(BRPeerManager *manager, const BRPeer peers[], size_t count,
                                                 void *info),
                               int (*networkIsReachable)(BRPeerManager *manager, void *info),
                               void *info)
{
}

// true if currently connected to at least one peer
int BRPeerMangerIsConnected(BRPeerManager *manager)
{
    return 0;
}

// connect to bitcoin peer-to-peer network
void BRPeerManagerConnect(BRPeerManager *manager)
{
}

// rescan blockchain for potentially missing transactions
void BRPeerManagerRescan(BRPeerManager *manager)
{
}

// current proof-of-work verified best block height
uint32_t BRPeerManagerLastBlockHeight(BRPeerManager *manager)
{
    return 0;
}

// the (unverified) best block height reported by connected peers
uint32_t BRPeerManagerEstimatedBlockHeight(BRPeerManager *manager)
{
    return 0;
}

// current network sync progress from 0 to 1
double BRPeerManagerSyncProgress(BRPeerManager *manager)
{
    return 0;
}

// returns the number of currently connected peers
size_t BRPeerManagerPeerCount(BRPeerManager *manager)
{
    return 0;
}

// publishes tx to bitcoin network
void BRPeerManagerPublishTx(BRTransaction *tx, void (*callback)(BRPeerManagerError error, void *info), void *info)
{
}

// number of connected peers that have relayed the transaction
size_t BRPeerMangaerRelayCount(UInt256 txHash)
{
    return 0;
}

// frees memory allocated for manager
void BRPeerManagerFree(BRPeerManager *manager)
{
}
