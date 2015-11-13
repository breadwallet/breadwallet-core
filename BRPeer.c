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
#include "BRRWLock.h"
#include "BRTypes.h"
#include "BRSet.h"
#include <float.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HEADER_LENGTH      24
#define MAX_MSG_LENGTH     0x02000000u
#define MAX_GETDATA_HASHES 50000
#define ENABLED_SERVICES   0     // we don't provide full blocks to remote nodes
#define PROTOCOL_VERSION   70002
#define MIN_PROTO_VERSION  70002 // peers earlier than this protocol version not supported (need v0.9 txFee relay rules)
#define LOCAL_HOST         0x7f000001u
#define CONNECT_TIMEOUT    3.0

typedef enum {
    inv_error = 0,
    inv_tx,
    inv_block,
    inv_merkleblock
} inv_type;

struct BRPeerContext {
    BRPeerStatus status;
    int waitingForNetwork;
    uint32_t version;
    uint64_t nonce;
    const char *useragent;
    uint32_t earliestKeyTime;
    uint32_t lastblock;
    double pingTime;
    int needsFilterUpdate;
    uint32_t currentBlockHeight;
    uint8_t *msgHeader, *msgPayload, *outBuffer;
    int sentVerack, gotVerack, sentGetaddr, sentFilter, sentGetdata, sentMempool, sentGetblocks;
    UInt256 *knownBlockHashes;
    BRSet *knownTxHashes, *currentBlockTxHashes;
    int socketFd;
    void *info;
    void (*connected)(void *info);
    void (*disconnected)(void *info, BRPeerError);
    void (*relayedPeers)(void *info, const BRPeer peers[], size_t count);
    void (*relayedTx)(void *info, const BRTransaction *tx);
    void (*hasTx)(void *info, UInt256 txHash);
    void (*rejectedTx)(void *info, UInt256 txHash, uint8_t code);
    void (*notfound)(void *info, const UInt256 txHashes[], size_t txCount, const UInt256 blockHashes[],
                     size_t blockCount);
    void (*relayedBlock)(void *info, const BRMerkleBlock *block);
    const BRTransaction *(*reqeustedTx)(void *info, UInt256 txHash);
    int (*networkIsReachable)(void *info);
    BRRWLock lock;
};

// call this before other BRPeer functions, set earliestKeyTime to wallet creation time to speed up initial sync
void BRPeerNewContext(BRPeer *peer, uint32_t earliestKeyTime)
{
    peer->context = calloc(1, sizeof(struct BRPeerContext));
    peer->context->earliestKeyTime = earliestKeyTime;
}

void BRPeerSetCallbacks(BRPeer *peer, void *info,
                        void (*connected)(void *info),
                        void (*disconnected)(void *info, BRPeerError error),
                        void (*relayedPeers)(void *info, const BRPeer peers[], size_t count),
                        void (*relayedTx)(void *info, const BRTransaction *tx),
                        void (*hasTx)(void *info, UInt256 txHash),
                        void (*rejectedTx)(void *info, UInt256 txHash, uint8_t code),
                        void (*relayedBlock)(void *info, const BRMerkleBlock *block),
                        void (*notfound)(void *info, const UInt256 txHashes[], size_t txCount,
                                         const UInt256 blockHashes[], size_t blockCount),
                        const BRTransaction *(*reqeustedTx)(void *info, UInt256 txHash),
                        int (*networkIsReachable)(void *info))
{
    struct BRPeerContext *ctx = peer->context;
    
    ctx->info = info;
    ctx->connected = connected;
    ctx->disconnected = disconnected;
    ctx->relayedPeers = relayedPeers;
    ctx->relayedTx = relayedTx;
    ctx->hasTx = hasTx;
    ctx->rejectedTx = rejectedTx;
    ctx->relayedBlock = relayedBlock;
    ctx->notfound = notfound;
    ctx->reqeustedTx = reqeustedTx;
    ctx->networkIsReachable = networkIsReachable;
}

// current connection status
BRPeerStatus BRPeerGetStatus(BRPeer *peer)
{
    return peer->context->status;
}

void BRPeerConnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;

    BRRWLockWrite(&ctx->lock);
    
    if (ctx->status != BRPeerStatusDisconnected && ! ctx->waitingForNetwork) {
        BRRWLockUnlock(&ctx->lock);
        return;
    }
    
    ctx->status = BRPeerStatusConnecting;
    ctx->pingTime = DBL_MAX;
    
    if (! ctx->networkIsReachable(ctx->info)) { // delay connect until network is reachable
        ctx->waitingForNetwork = 1;
        BRRWLockUnlock(&ctx->lock);
        return;
    }
    
    ctx->waitingForNetwork = 0;
    array_new(ctx->msgHeader, HEADER_LENGTH);
    array_new(ctx->msgPayload, 1000);
    array_new(ctx->outBuffer, 1000);
    array_new(ctx->knownBlockHashes, MAX_GETDATA_HASHES);
    ctx->knownTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 100);
    ctx->currentBlockTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 100);

    // XXXX do connect things and stuff

    BRRWLockUnlock(&ctx->lock);
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

void BRPeerSendPing(BRPeer *peer, void *info, void (*pongCallback)(void *info, int success))
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
