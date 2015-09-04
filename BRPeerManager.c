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
    uint32_t earliestKeyTime;
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
    void (*publishedCallback)();
    size_t publishedCount;
};
