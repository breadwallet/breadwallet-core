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
#include "BRAddress.h"
#include "BRSet.h"
#include "BRRWLock.h"
#include "BRArray.h"
#include "BRHash.h"
#include "BRInt.h"
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
#define LOCAL_HOST         ((UInt128) { .u32 = { 0, 0, be32(0xffff), be32(0x7f000001) } })
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
    char *useragent;
    uint32_t earliestKeyTime;
    uint32_t lastblock;
    double startTime;
    double pingTime;
    int needsFilterUpdate;
    uint32_t currentBlockHeight;
    int sentVerack, gotVerack, sentGetaddr, sentFilter, sentGetdata, sentMempool, sentGetblocks;
    UInt256 *knownBlockHashes;
    BRSet *knownTxHashes, *currentBlockTxHashes;
    BRMerkleBlock *currentBlock;
    int socket;
    void *info;
    void (*connected)(void *info);
    void (*disconnected)(void *info, int error);
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

void BRPeerFreeContext(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;
    
    if (ctx) {
        peer->context = NULL;
        if (ctx->useragent) array_free(ctx->useragent);
        if (ctx->knownBlockHashes) array_free(ctx->knownBlockHashes);
        if (ctx->knownTxHashes) BRSetFree(ctx->knownTxHashes);
        if (ctx->currentBlockTxHashes) BRSetFree(ctx->currentBlockTxHashes);
        BRRWLockDestroy(&ctx->lock);
        free(ctx);
    }
}

static void BRPeerDidConnect(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;

    if (ctx->status == BRPeerStatusConnecting && ctx->sentVerack && ctx->gotVerack) {
        peer_log(peer, "handshake completed");
        // TODO: XXX cancel pending handshake timeout
        ctx->status = BRPeerStatusConnected;
        if (ctx->connected) ctx->connected(ctx->info);
    }
}

static void BRPeerErrorDisconnect(BRPeer *peer, int error)
{
    struct BRPeerContext *ctx = peer->context;
    
    // call shutdown() to causes the reader thread to exit before calling close() to release the socket descriptor,
    // otherwise the descriptor can get immediately re-used, and any subsequent writes will result in file corruption
    BRRWLockRead(&ctx->lock);
    if (ctx->socket >= 0) shutdown(ctx->socket, SHUT_RDWR);
    BRRWLockUnlock(&ctx->lock);
    BRRWLockWrite(&ctx->lock); // this will block until all socket writes are done
    if (ctx->socket >= 0) close(ctx->socket);
    ctx->socket = -1;
    BRRWLockUnlock(&ctx->lock);
    BRRWLockRead(&ctx->lock);
    if (ctx->disconnected) ctx->disconnected(peer, error);
    BRRWLockUnlock(&ctx->lock);
    BRPeerFreeContext(peer);
}

static int BRPeerAcceptVersionMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    struct BRPeerContext *ctx = peer->context;
    int r = 1;
    size_t off = 0, strLen = 0, l = 0;
    uint64_t recvServices, fromServices, nonce;
    UInt128 recvAddr, fromAddr;
    uint16_t recvPort, fromPort;
    
    if (len < 85) {
        peer_log(peer, "malformed version message, length is %zu, should be > 84", len);
        r = 0;
    }
    else {
        ctx->version = le32(*(uint32_t *)(msg + off));
        off += sizeof(uint32_t);
    
        if (ctx->version < MIN_PROTO_VERSION) {
            peer_log(peer, "protocol version %u not supported", ctx->version);
            r = 0;
        }
        else {
            peer->services = le64(*(uint64_t *)(msg + off));
            off += sizeof(uint64_t);
            peer->timestamp = le64(*(uint64_t *)(msg + off));
            off += sizeof(uint64_t);
            recvServices = le64(*(uint64_t *)(msg + off));
            off += sizeof(uint64_t);
            recvAddr = *(UInt128 *)(msg + off);
            off += sizeof(UInt128);
            recvPort = be16(*(uint16_t *)(msg + off));
            off += sizeof(uint16_t);
            fromServices = le64(*(uint64_t *)(msg + off));
            off += sizeof(uint64_t);
            fromAddr = *(UInt128 *)(msg + off);
            off += sizeof(UInt128);
            fromPort = be16(*(uint16_t *)(msg + off));
            off += sizeof(uint16_t);
            nonce = le64(*(uint64_t *)(msg + off));
            off += sizeof(uint64_t);
            strLen = BRVarInt(msg + off, len - off, &l);
            off += l;

            if (len < off + strLen + sizeof(uint32_t)) {
                peer_log(peer, "malformed version message, length is %zu, should be %zu", len,
                         off + strLen + sizeof(uint32_t));
                r = 0;
            }
            else {
                array_clear(ctx->useragent);
                array_add_array(ctx->useragent, msg + off, strLen);
                array_add(ctx->useragent, '\0');
                off += strLen;
                ctx->lastblock = le32(*(uint32_t *)(msg + off));
                off += sizeof(uint32_t);
                peer_log(peer, "got version %u, useragent:\"%s\"", ctx->version, ctx->useragent);
                BRPeerSendVerackMessage(peer);
            }
        }
    }
    
    return r;
}

static int BRPeerAcceptVerackMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    struct BRPeerContext *ctx = peer->context;
    int r = 1;

    if (ctx->gotVerack) {
        peer_log(peer, "got unexpected verack");
    }
    else {
        ctx->pingTime = (double)clock()/CLOCKS_PER_SEC - ctx->startTime; // use verack time as initial ping time
        ctx->startTime = 0;
        peer_log(peer, "got verack in %fs", ctx->pingTime);
        ctx->gotVerack = 1;
        BRPeerDidConnect(peer);
    }
    
    return r;
}

static int BRPeerAcceptAddrMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptInvMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptTxMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptHeadersMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptGetaddrMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptGetdataMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptNotfoundMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptPingMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptPongMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptMerkleblockMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptRejectMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    int r = 1;
    
    return r;
}

static int BRPeerAcceptMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type)
{
    struct BRPeerContext *ctx = peer->context;
    UInt256 hash;
    int r = 1;
    
    if (ctx->currentBlock && strncmp(MSG_TX, type, 12) != 0) { // if we receive non-tx message, merkleblock is done
        hash = ctx->currentBlock->blockHash;
        ctx->currentBlock = NULL;
        BRSetClear(ctx->currentBlockTxHashes);
        peer_log(peer, "incomplete merkleblock %s, expected %zu more tx, got %s", uint256_hex_str(hash),
                 BRSetCount(ctx->currentBlockTxHashes), type);
        r = 0;
    }
    else if (strncmp(MSG_VERSION, type, 12) == 0) r = BRPeerAcceptVersionMessage(peer, msg, len);
    else if (strncmp(MSG_VERACK, type, 12) == 0) r = BRPeerAcceptVerackMessage(peer, msg, len);
    else if (strncmp(MSG_ADDR, type, 12) == 0) r = BRPeerAcceptAddrMessage(peer, msg, len);
    else if (strncmp(MSG_INV, type, 12) == 0) r = BRPeerAcceptInvMessage(peer, msg, len);
    else if (strncmp(MSG_TX, type, 12) == 0) r = BRPeerAcceptTxMessage(peer, msg, len);
    else if (strncmp(MSG_HEADERS, type, 12) == 0) r = BRPeerAcceptHeadersMessage(peer, msg, len);
    else if (strncmp(MSG_GETADDR, type, 12) == 0) r = BRPeerAcceptGetaddrMessage(peer, msg, len);
    else if (strncmp(MSG_GETDATA, type, 12) == 0) r = BRPeerAcceptGetdataMessage(peer, msg, len);
    else if (strncmp(MSG_NOTFOUND, type, 12) == 0) r = BRPeerAcceptNotfoundMessage(peer, msg, len);
    else if (strncmp(MSG_PING, type, 12) == 0) r = BRPeerAcceptPingMessage(peer, msg, len);
    else if (strncmp(MSG_PONG, type, 12) == 0) r = BRPeerAcceptPongMessage(peer, msg, len);
    else if (strncmp(MSG_MERKLEBLOCK, type, 12) == 0) r = BRPeerAcceptMerkleblockMessage(peer, msg, len);
    else if (strncmp(MSG_REJECT, type, 12) == 0) r = BRPeerAcceptRejectMessage(peer, msg, len);
    else peer_log(peer, "dropping %s, length %zu, not implemented", type, len);

    return r;
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
        serv_addr.sin_addr.s_addr = peer->address.u32[3]; // already in network byte order
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

        if (r) peer_log(peer, "socket connected");
        fcntl(socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r) peer_log(peer, "connect error: %s", strerror(error));
    return r;
}

static void *BRPeerThreadRoutine(void *peer)
{
    struct BRPeerContext *ctx = ((BRPeer *)peer)->context;
    int error = 0;

    BRRWLockWrite(&ctx->lock);
    
    if (BRPeerOpenSocket(peer, CONNECT_TIMEOUT)) {
        ctx->startTime = (double)clock()/CLOCKS_PER_SEC;
        // TODO: XXX start handshake timeout
        BRRWLockUnlock(&ctx->lock);
        BRPeerSendVersionMessage(peer);
        BRRWLockRead(&ctx->lock);
        
        uint8_t header[HEADER_LENGTH];
        size_t len = 0;
        ssize_t n = 0;
        
        while (! error && n >= 0) {
            n = 0;
            len = 0;

            while (n >= 0 && len < HEADER_LENGTH) {
                n = read(ctx->socket, header + len, sizeof(header) - len);
                if (n >= 0) len += n;
                
                while (len >= sizeof(uint32_t) && *(uint32_t *)header != le32(MAGIC_NUMBER)) {
                    memmove(header, header + 1, --len); // consume one byte at a time until we find the magic number
                }
            }
        
            if (n < 0) {
                peer_log((BRPeer *)peer, "%s", strerror(errno));
                error = errno;
            }
            else if (header[15] != 0) { // verify header type field is NULL terminated
                peer_log((BRPeer *)peer, "malformed message header: type not NULL terminated");
                error = EPROTO;
            }
            else if (len == HEADER_LENGTH) {
                const char *type = (const char *)(header + 4);
                uint32_t msgLen = le32(*(uint32_t *)(header + 16));
                uint32_t checksum = *(uint32_t *)(header + 20);
                
                if (msgLen > MAX_MSG_LENGTH) { // check message length
                    peer_log((BRPeer *)peer, "error reading %s, message length %u is too long", type, msgLen);
                    error = EPROTO;
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
                        error = errno;
                    }
                    else if (len == msgLen) {
                        BRSHA256_2(&hash, payload, msgLen);
                        
                        if (hash.u32[0] != checksum) { // verify checksum
                            peer_log((BRPeer *)peer, "error reading %s, invalid checksum %x, expected %x, payload "
                                     "length:%u, SHA256_2:%s", type, be32(hash.u32[0]), be32(checksum), msgLen,
                                     uint256_hex_str(hash));
                            error = EPROTO;
                        }
                        else if (! BRPeerAcceptMessage(peer, payload, msgLen, type)) error = EPROTO;
                    }
                }
            }
        }
    }

    BRRWLockUnlock(&ctx->lock);
    BRPeerErrorDisconnect(peer, error);
    return NULL; // detached threads don't need to return a value
}

void BRPeerSetCallbacks(BRPeer *peer, void *info,
                        void (*connected)(void *info),
                        void (*disconnected)(void *info, int error),
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
BRPeerStatus BRPeerConnectStatus(BRPeer *peer)
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
    
        if (ctx->networkIsReachable && ! ctx->networkIsReachable(ctx->info)) { // delay until network is reachable
            ctx->waitingForNetwork = 1;
            BRRWLockUnlock(&ctx->lock);
        }
        else {
            ctx->waitingForNetwork = 0;
            array_new(ctx->knownBlockHashes, 10);
            array_new(ctx->useragent, 40);
            ctx->knownTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            ctx->currentBlockTxHashes = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            ctx->socket = socket(AF_INET, SOCK_STREAM, 0);
            if (ctx->socket >= 0) setsockopt(ctx->socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
            if (ctx->socket >= 0) setsockopt(ctx->socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
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
    BRPeerErrorDisconnect(peer, 0); // disconnect with no error
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

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

void BRPeerSendMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type)
{
    if (len > MAX_MSG_LENGTH) {
        peer_log(peer, "failed to send %s, length %zu is too long", type, len);
    }
    else {
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
        peer_log(peer, "sending %s", type);
        
        struct BRPeerContext *ctx = peer->context;
        int error = 0;
        
        BRRWLockRead(&ctx->lock); // we only need a read lock on the peer to write to the socket
        if (ctx->socket >= 0 && send(ctx->socket, buf, HEADER_LENGTH + len, MSG_NOSIGNAL) < 0) error = errno;
        BRRWLockUnlock(&ctx->lock);
        if (error) BRPeerErrorDisconnect(peer, error);
    }
}

void BRPeerSendVersionMessage(BRPeer *peer)
{
    struct BRPeerContext *ctx = peer->context;
    size_t off = 0, userAgentLen = strlen(USER_AGENT);
    uint8_t msg[80 + BRVarIntSize(userAgentLen) + userAgentLen + 5];
    
    *(uint32_t *)(msg + off) = le32(PROTOCOL_VERSION); // version
    off += sizeof(uint32_t);
    *(uint64_t *)(msg + off) = le64(ENABLED_SERVICES); // services
    off += sizeof(uint64_t);
    *(uint64_t *)(msg + off) = le64(time(NULL)); // timestamp
    off += sizeof(uint64_t);
    *(uint64_t *)(msg + off) = le64(peer->services); // services of remote peer
    off += sizeof(uint64_t);
    *(UInt128 *)(msg + off) = peer->address; // IPv6 address of remote peer
    off += sizeof(UInt128);
    *(uint16_t *)(msg + off) = be16(peer->port); // port of remote peer
    off += sizeof(uint16_t);
    *(uint64_t *)(msg + off) = le64(ENABLED_SERVICES); // services
    off += sizeof(uint64_t);
    *(UInt128 *)(msg + off) = LOCAL_HOST; // IPv4 mapped IPv6 header
    off += sizeof(UInt128);
    *(uint16_t *)(msg + off) = be16(STANDARD_PORT);
    off += sizeof(uint16_t);
    BRRWLockWrite(&ctx->lock);
    ctx->nonce = ((uint64_t)BRRand(0) << 32) | (uint64_t)BRRand(0); // random nonce
    *(uint64_t *)(msg + off) = le64(ctx->nonce);
    BRRWLockUnlock(&ctx->lock);
    off += sizeof(uint64_t);
    off += BRVarIntSet(msg + off, sizeof(msg) - off, userAgentLen);
    strncpy((char *)(msg + off), USER_AGENT, userAgentLen); // user agent string
    off += userAgentLen;
    *(uint32_t *)(msg + off) = le32(0); // last block received
    off += sizeof(uint32_t);
    msg[off++] = 0; // relay transactions (no for SPV bloom filter mode)
    BRPeerSendMessage(peer, msg, sizeof(msg), MSG_VERSION);
}

void BRPeerSendVerackMessage(BRPeer *peer)
{
    BRPeerSendMessage(peer, NULL, 0, MSG_VERACK);
    peer->context->sentVerack = 1;
    BRPeerDidConnect(peer);
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
