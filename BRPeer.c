//
//  BRPeer.c
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

#include "BRPeer.h"

struct BRPeerContext {
    BRPeerStatus status;
    uint32_t version;
    uint64_t nonce;
    const char *useragent;
    uint32_t earliestKeyTime;
    uint32_t lastblock;
    double pingTime;
    int needsFilterUpdate;
    uint32_t currentBlockHeight;
    void (*connected)(BRPeer *peer, void *info);
    void (*disconnected)(BRPeer *peer, BRPeerError, void *info);
    void (*relayedPeers)(BRPeer *peer, const BRPeer peers[], size_t count, void *info);
    void (*relayedTx)(BRPeer *peer, const BRTransaction *tx, void *info);
    void (*hasTx)(BRPeer *peer, UInt256 txHash, void *info);
    void (*rejectedTx)(BRPeer *peer, UInt256 txHash, uint8_t code, void *info);
    void (*relayedBlock)(BRPeer *peer, const BRMerkleBlock *block, void *info);
    const BRTransaction *(*reqeustedTx)(BRPeer *peer, UInt256 txHash, void *info);
    int (*networkIsReachable)(BRPeer *peer, void *info);
    void *info;
};

// call this before other BRPeer functions, set earliestKeyTime to wallet creation time to speed up initial sync
void BRPeerNewContext(BRPeer *peer, uint32_t earliestKeyTime)
{
}

void BRPeerSetCallbacks(BRPeer *peer,
                        void (*connected)(BRPeer *peer, void *info),
                        void (*disconnected)(BRPeer *peer, BRPeerError error, void *info),
                        void (*relayedPeers)(BRPeer *peer, const BRPeer peers[], size_t count, void *info),
                        void (*relayedTx)(BRPeer *peer, const BRTransaction *tx, void *info),
                        void (*hasTx)(BRPeer *peer, UInt256 txHash, void *info),
                        void (*rejectedTx)(BRPeer *peer, UInt256 txHash, uint8_t code, void *info),
                        void (*relayedBlock)(BRPeer *peer, const BRMerkleBlock *block, void *info),
                        const BRTransaction *(*reqeustedTx)(BRPeer *peer, UInt256 txHash, void *info),
                        int (*networkIsReachable)(BRPeer *peer, void *info),
                        void *info)
{
}

// current connection status
BRPeerStatus BRPeerGetStatus(BRPeer *peer)
{
    return 0;
}

void BRPeerConnect(BRPeer *peer)
{
}

void BRPeerDisconnect(BRPeer *peer)
{
}

// call this when wallet addresses need to be added to bloom filter
void BRPeerNeedsFilterUpdate(BRPeer *peer)
{
}

// call this when local block height changes (helps detect tarpit nodes)
void BRPeerSetCurrentBlockHeight(BRPeer *peer, uint32_t currentBlockHeight)
{
}

// connected peer version number
uint32_t BRPeerVersion(BRPeer *peer)
{
    return 0;
}

// connected peer user agent string
const char *BRPeerUserAgent(BRPeer *peer)
{
    return NULL;
}

// best block height reported by connected peer
uint32_t BRPeerLastBlock(BRPeer *peer)
{
    return 0;
}

// ping time for connected peer
double BRPeerPingTime(BRPeer *peer)
{
    return 0;
}

void BRPeerSendMessage(BRPeer *peer, const uint8_t *message, size_t len, const char *type)
{
}

void BRPeerSendFilterload(BRPeer *peer, const uint8_t *filter, size_t len)
{
}

void BRPeerSendMempool(BRPeer *peer)
{
}

void BRPeerSendGetheaders(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop)
{
}

void BRPeerSendGetblocks(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop)
{
}

void BRPeerSendInv(BRPeer *peer, const UInt256 txHashes[], size_t count)
{
}

void BRPeerSendGetdata(BRPeer *peer, const UInt256 txHashes[], size_t txCount, const UInt256 blockHashes[],
                       size_t blockCount)
{
}

void BRPeerSendGetaddr(BRPeer *peer)
{
}

void BRPeerSendPing(BRPeer *peer, void (*pongCallback)(BRPeer *peer, int success, void *info), void *info)
{
}

// useful to get additional tx after a bloom filter update
void BRPeerRerequestBlocks(BRPeer *peer, UInt256 fromBlock)
{
}

// frees memory allocated for peer after calling BRPeerCreateContext()
void BRPeerFreeContext(BRPeer *peer)
{
}
