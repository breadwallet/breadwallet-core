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
#include "BRHash.h"
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#if BITCOIN_TESTNET
#define MAGIC_NUMBER 0x0709110b
#else
#define MAGIC_NUMBER 0xd9b4bef9
#endif
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
                 be64(h.u64[0]), be64(h.u64[1]), be64(h.u64[2]), be64(h.u64[3]), BRSetCount(ctx->currentBlockTxHashes),
                 type);
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

static int BRPeerOpenSocket(BRPeer *peer, double timeout)
{
    struct sockaddr_in serv_addr;
    struct timeval tv;
    fd_set fds;
    socklen_t socklen;
    int socket = peer->context->socket;
    int arg, count, error = 0, r = 1;

    arg = fcntl(socket, F_GETFL, NULL);
    if (arg < 0 || fcntl(socket, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set the socket non-blocking
    if (! r) error = errno;

    if (r) {
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = peer->address.u32[3]; // already network byte order
        serv_addr.sin_port = htons(peer->port);
        if (connect(socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) error = errno;

        if (error == EINPROGRESS) {
            error = 0;
            tv.tv_sec = timeout;
            tv.tv_usec = (long)(timeout*1000000) % 1000000;
            FD_ZERO(&fds);
            FD_SET(socket, &fds);
            count = select(socket + 1, NULL, &fds, NULL, &tv);

            if (count <= 0 || getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &socklen) < 0 || error) {
                if (count == 0) error = ETIMEDOUT;
                if (count < 0 || ! error) error = errno;
                r = 0;
            }
        }

        fcntl(socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r) peer_log(peer, "connect error: %s", strerror(error));
    return r;
}

static void *BRPeerThreadRoutine(void *peer)
{
    struct BRPeerContext *ctx = ((BRPeer *)peer)->context;

    BRRWLockWrite(&ctx->lock);

    if (BRPeerOpenSocket(peer, CONNECT_TIMEOUT)) {
        ctx->status = BRPeerStatusConnected;
        peer_log((BRPeer *)peer, "connected");
        BRRWLockUnlock(&ctx->lock);
        BRRWLockRead(&ctx->lock);
        if (ctx->connected) ctx->connected(ctx->info);
        
        uint8_t header[HEADER_LENGTH];
        size_t len = 0;
        ssize_t n = 0;
        
        while (n >= 0) {
            while (n >= 0 && len < HEADER_LENGTH) {
                n = read(ctx->socket, header + len, sizeof(header) - len);
                if (n >= 0) len += n;
                
                while (len >= sizeof(uint32_t) && *(uint32_t *)header != le32(MAGIC_NUMBER)) {
                    memmove(header, header + 1, --len); // consume one byte at a time until we find the magic number
                }
            }
        
            if (n < 0) {
                peer_log((BRPeer *)peer, "%s", strerror(errno));
            }
            else if (header[15] != 0) { // verify header type field is NULL terminated
                peer_log((BRPeer *)peer, "malformed message header: type not NULL terminated");
            }
            else if (len == HEADER_LENGTH) {
                const char *type = (const char *)(header + 4);
                uint32_t msgLen = le32(*(uint32_t *)(header + 16));
                uint32_t checksum = *(uint32_t *)(header + 20);
                
                if (msgLen > MAX_MSG_LENGTH) { // check message length
                    peer_log((BRPeer *)peer, "error reading %s, message length %u is too long", type, msgLen);
                }
                else {
                    uint8_t payload[msgLen];
                    UInt256 hash;
                    
                    n = 0;
                    len = 0;
                    
                    while (n >= 0 && len < msgLen) {
                        n = read(ctx->socket, payload + len, sizeof(payload) - len);
                        if (n >= 0) len += n;
                    }
                    
                    if (n < 0) {
                        peer_log((BRPeer *)peer, "%s", strerror(errno));
                    }
                    else if (len == msgLen) {
                        BRSHA256_2(&hash, payload, msgLen);
                        
                        if (hash.u32[0] != checksum) { // verify checksum
                            peer_log((BRPeer *)peer, "error reading %s, invalid checksum %x, expected %x, payload "
                                     "length:%u, SHA256_2:%llX%llX%llX%llX", type, be32(hash.u32[0]), be32(checksum),
                                     msgLen, be64(hash.u64[0]), be64(hash.u64[1]), be64(hash.u64[2]),
                                     be64(hash.u64[3]));
                        }
                        else BRPeerAcceptMessage(peer, payload, msgLen, type);
                    }
                }
            }
        }
    }

    BRRWLockUnlock(&ctx->lock);
    BRPeerDisconnect(peer);
    return NULL; // detached threads don't need to return a value
}

struct BRPeerContext *BRPeerNewContext(BRPeer *peer)
{
    struct BRPeerContext *ctx = calloc(1, sizeof(struct BRPeerContext));
    
    if (peer->address.u64[0] == 0 && peer->address.u16[4] == 0xffff) {
        inet_ntop(AF_INET, &peer->address.u32[3], ctx->host, sizeof(ctx->host));
    }
    else inet_ntop(AF_INET6, &peer->address, ctx->host, sizeof(ctx->host));
    
    ctx->socket = -1;
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

void BRPeerConnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;
    pthread_attr_t attr;
    int on = 1;
    
    if (! ctx) ctx = BRPeerNewContext(peer);
    BRRWLockWrite(&ctx->lock);
    
    if (ctx->status == BRPeerStatusDisconnected || ctx->waitingForNetwork) {
        ctx->status = BRPeerStatusConnecting;
        ctx->pingTime = DBL_MAX;
    
        if (! ctx->networkIsReachable(ctx->info)) { // delay connect until network is reachable
            ctx->waitingForNetwork = 1;
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

            if (ctx->socket >= 0) {
                setsockopt(ctx->socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
                setsockopt(ctx->socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
            }
            
            BRRWLockUnlock(&ctx->lock);
            BRRWLockRead(&ctx->lock);

            if (ctx->socket < 0 || pthread_attr_init(&attr) != 0) {
                BRRWLockUnlock(&ctx->lock);
                BRPeerFreeContext(peer);
            }
            else if (pthread_attr_setstacksize(&attr, 512*1024) != 0 || // set stack size since there's no standard
                     // set thread as detached so it will free resources immediately on exit without waiting for join
                     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
                     pthread_create(&ctx->thread, &attr, BRPeerThreadRoutine, peer) != 0) {
                peer_log(peer, "error creating thread");
                BRRWLockUnlock(&ctx->lock);
                BRPeerFreeContext(peer);
                pthread_attr_destroy(&attr);
            }
            else BRRWLockUnlock(&ctx->lock);
        }
    }
}

void BRPeerDisconnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;

    // call shutdown() to causes the reader thread to exit before calling close() to release the socket descriptor,
    // otherwise the descriptor can get immediately re-used, and any subsequent writes will result in file corruption
    BRRWLockRead(&ctx->lock);
    if (ctx->socket >= 0) shutdown(ctx->socket, SHUT_RDWR);
    BRRWLockUnlock(&ctx->lock);
    BRRWLockWrite(&ctx->lock); // this will block until all read locks are released
    if (ctx->socket >= 0) close(ctx->socket);
    ctx->socket = -1;
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
    if (len > MAX_MSG_LENGTH) {
        peer_log(peer, "failed to send %s, length %zu is too long", type, len);
        return;
    }

    uint8_t buf[HEADER_LENGTH + len], hash[32];
    size_t off = 0;
    
    *(uint32_t *)(buf + off) = le32(MAGIC_NUMBER);
    off += sizeof(uint32_t);
    strncpy((char *)buf + off, type, 12);
    off += 12;
    *(uint32_t *)(buf + off) = le32((uint32_t)len);
    off += sizeof(uint32_t);
    BRSHA256_2(hash, msg, len);
    *(uint32_t *)(buf + off) = *(uint32_t *)hash;
    off += sizeof(uint32_t);
    memcpy(buf + off, msg, len);

    struct BRPeerContext *ctx = peer->context;
    
    BRRWLockRead(&ctx->lock); // we only need a read lock on the peer to write to the socket
    if (ctx->socket >= 0) write(ctx->socket, buf, HEADER_LENGTH + len);
    BRRWLockUnlock(&ctx->lock);
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
