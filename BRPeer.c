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
#include "BRMerkleBlock.h"
#include "BRSet.h"
#include "BRRWLock.h"
#include "BRArray.h"
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define HEADER_LENGTH      24
#define MAX_MSG_LENGTH     0x02000000
#define MAX_GETDATA_HASHES 50000
#define ENABLED_SERVICES   0     // we don't provide full blocks to remote nodes
#define PROTOCOL_VERSION   70002
#define MIN_PROTO_VERSION  70002 // peers earlier than this protocol version not supported (need v0.9 txFee relay rules)
#define LOCAL_HOST         0x7f000001
#define CONNECT_TIMEOUT    3.0

#define peer_log(peer, ...)\
    printf("%s:%u " _va_first(__VA_ARGS__, NULL) "\n", (peer)->context->host, (peer)->port, _va_rest(__VA_ARGS__, NULL))
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__

typedef enum {
    inv_error = 0,
    inv_tx,
    inv_block,
    inv_merkleblock
} inv_type;

struct BRPeerContext {
    char host[INET6_ADDRSTRLEN];
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
    BRMerkleBlock *currentBlock;
    int socket;
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
    pthread_t thread;
};

static void BRPeerAcceptVersionMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptVerackMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptAddrMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptInvMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptTxMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptHeadersMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptGetaddrMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptGetdataMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptNotfoundMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptPingMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptPongMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptMerkleblockMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptRejectMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
}

static void BRPeerAcceptMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type)
{
    struct BRPeerContext *ctx = peer->context;
    UInt256 h;
    
    if (ctx->currentBlock && strncmp(MSG_TX, type, 12) != 0) { // if we receive non-tx message, merkleblock is done
        h = ctx->currentBlock->blockHash;
        peer_log(peer, "incomplete merkleblock %llX%llX%llX%llX, expected %zu more tx, got %s",
                 h.u64[0], h.u64[1], h.u64[2], h.u64[3], BRSetCount(ctx->currentBlockTxHashes), type);
        ctx->currentBlock = NULL;
        BRSetClear(ctx->currentBlockTxHashes);
        return;
    }
    
    if (strncmp(MSG_VERSION, type, 12) == 0) BRPeerAcceptVersionMessage(peer, msg, len);
    else if (strncmp(MSG_VERACK, type, 12) == 0) BRPeerAcceptVerackMessage(peer, msg, len);
    else if (strncmp(MSG_ADDR, type, 12) == 0) BRPeerAcceptAddrMessage(peer, msg, len);
    else if (strncmp(MSG_INV, type, 12) == 0) BRPeerAcceptInvMessage(peer, msg, len);
    else if (strncmp(MSG_TX, type, 12) == 0) BRPeerAcceptTxMessage(peer, msg, len);
    else if (strncmp(MSG_HEADERS, type, 12) == 0) BRPeerAcceptHeadersMessage(peer, msg, len);
    else if (strncmp(MSG_GETADDR, type, 12) == 0) BRPeerAcceptGetaddrMessage(peer, msg, len);
    else if (strncmp(MSG_GETDATA, type, 12) == 0) BRPeerAcceptGetdataMessage(peer, msg, len);
    else if (strncmp(MSG_NOTFOUND, type, 12) == 0) BRPeerAcceptNotfoundMessage(peer, msg, len);
    else if (strncmp(MSG_PING, type, 12) == 0) BRPeerAcceptPingMessage(peer, msg, len);
    else if (strncmp(MSG_PONG, type, 12) == 0) BRPeerAcceptPongMessage(peer, msg, len);
    else if (strncmp(MSG_MERKLEBLOCK, type, 12) == 0) BRPeerAcceptMerkleblockMessage(peer, msg, len);
    else if (strncmp(MSG_REJECT, type, 12) == 0) BRPeerAcceptRejectMessage(peer, msg, len);
    else peer_log(peer, "dropping %s, length %zu, not implemented", type, len);
}

struct BRPeerContext *BRPeerNewContext(BRPeer *peer)
{
    struct BRPeerContext *ctx = calloc(1, sizeof(struct BRPeerContext));
    
    if (peer->address.u64[0] == 0 && peer->address.u16[4] == 0xffff) {
        inet_ntop(AF_INET, &peer->address.u32[3], ctx->host, sizeof(ctx->host));
    }
    else inet_ntop(AF_INET6, &peer->address, ctx->host, sizeof(ctx->host));

    BRRWLockInit(&ctx->lock);
    peer->context = ctx;
    return ctx;
}

// frees memory allocated for peer after calling BRPeerCreateContext()
void BRPeerFreeContext(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;
    
    if (ctx) {
        peer->context = NULL;
        if (ctx->msgHeader) array_free(ctx->msgHeader);
        if (ctx->msgPayload) array_free(ctx->msgPayload);
        if (ctx->outBuffer) array_free(ctx->outBuffer);
        if (ctx->knownBlockHashes) array_free(ctx->knownBlockHashes);
        if (ctx->knownTxHashes) BRSetFree(ctx->knownTxHashes);
        if (ctx->currentBlockTxHashes) BRSetFree(ctx->currentBlockTxHashes);
        BRRWLockDestroy(&ctx->lock);
        free(ctx);
    }
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
    
    if (! ctx) ctx = BRPeerNewContext(peer);
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
    return (peer->context) ? peer->context->status : BRPeerStatusDisconnected;
}

static void *BRPeerThreadRoutine(void *peer)
{
    struct BRPeerContext *ctx = ((BRPeer *)peer)->context;
    struct sockaddr_in serv_addr;
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = ((BRPeer *)peer)->address.u32[3]; // already network byte order
    serv_addr.sin_port = htons(((BRPeer *)peer)->port);
    
    // setsockopt SO_KEEPALIVE, SO_NOSIGPIPE?
    // In iOS, using sockets directly using POSIX functions does not automatically activate the device’s
    // cellular modem or on-demand VPN

    if (connect(ctx->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        BRPeerFreeContext(peer); // connect error
    }
    else {
        if (ctx->connected) ctx->connected(ctx->info);
    
        // n = write(ctx->socket, buf, bufLen);
        // if (n < 0) peer_log(peer, "ERROR writing to socket");
        // n = read(ctx->socket, buf, bufLen);
        // if (n < 0) peer_log(peer, "ERROR reading from socket");
        
        BRPeerDisconnect(peer);
    }
    
    return NULL;
}

void BRPeerConnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;

    if (! ctx) ctx = BRPeerNewContext(peer);
    BRRWLockWrite(&ctx->lock);
    
    if (ctx->status == BRPeerStatusDisconnected || ctx->waitingForNetwork) {
        ctx->status = BRPeerStatusConnecting;
        ctx->pingTime = DBL_MAX;
    
        if (! ctx->networkIsReachable(ctx->info)) {
            ctx->waitingForNetwork = 1; // delay connect until network is reachable
            BRRWLockUnlock(&ctx->lock);
        }
        else {
            ctx->waitingForNetwork = 0;
            array_new(ctx->msgHeader, HEADER_LENGTH);
            array_new(ctx->msgPayload, 1000);
            array_new(ctx->outBuffer, 1000);
            array_new(ctx->knownBlockHashes, 10);
            ctx->knownTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            ctx->currentBlockTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            ctx->socket = socket(AF_INET, SOCK_STREAM, 0);
            BRRWLockUnlock(&ctx->lock);
        
            if (ctx->socket >= 0) {
                BRPeerFreeContext(peer);
            }
            else {
                pthread_attr_t attr;
        
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

                if (pthread_create(&ctx->thread, &attr, BRPeerThreadRoutine, peer) != 0) {
                    BRPeerFreeContext(peer);
                }
                
                pthread_attr_destroy(&attr);
            }
        }
    }
}

void BRPeerDisconnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;
    
    BRRWLockWrite(&ctx->lock);
    
    close(ctx->socket);
    
    BRRWLockUnlock(&ctx->lock);
    BRPeerFreeContext(peer);
}

// set earliestKeyTime to wallet creation time in order to speed up initial sync
void BRPeerSetEarliestKeyTime(BRPeer *peer, uint32_t earliestKeyTime)
{
    if (! peer->context) BRPeerNewContext(peer);
    peer->context->earliestKeyTime = earliestKeyTime;
}

// call this when local block height changes (helps detect tarpit nodes)
void BRPeerSetCurrentBlockHeight(BRPeer *peer, uint32_t currentBlockHeight)
{
    if (! peer->context) BRPeerNewContext(peer);
    peer->context->currentBlockHeight = currentBlockHeight;
}

// call this when wallet addresses need to be added to bloom filter
void BRPeerSetNeedsFilterUpdate(BRPeer *peer)
{
    if (peer->context) peer->context->needsFilterUpdate = 1;
}

// connected peer version number
uint32_t BRPeerVersion(BRPeer *peer)
{
    return (peer->context) ? peer->context->version : 0;
}

// connected peer user agent string
const char *BRPeerUserAgent(BRPeer *peer)
{
    return (peer->context) ? peer->context->useragent : NULL;
}

// best block height reported by connected peer
uint32_t BRPeerLastBlock(BRPeer *peer)
{
    return (peer->context) ? peer->context->lastblock : 0;
}

// ping time for connected peer
double BRPeerPingTime(BRPeer *peer)
{
    return (peer->context) ? peer->context->pingTime : DBL_MAX;
}

void BRPeerSendMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type)
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
