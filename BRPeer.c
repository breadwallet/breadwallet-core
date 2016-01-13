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
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>	
#include <arpa/inet.h>

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

// the standard blockchain download protocol works as follows (for SPV mode):
// - local peer sends getblocks
// - remote peer reponds with inv containing up to 500 block hashes
// - local peer sends getdata with the block hashes
// - remote peer responds with multiple merkleblock and tx messages
// - remote peer sends inv containg 1 hash, of the most recent block
// - local peer sends getdata with the most recent block hash
// - remote peer responds with merkleblock
// - if local peer can't connect the most recent block to the chain (because it started more than 500 blocks behind), go
//   back to first step and repeat until entire chain is downloaded
//
// we modify this sequence to improve sync performance and handle adding bip32 addresses to the bloom filter as needed:
// - local peer sends getheaders
// - remote peer responds with up to 2000 headers
// - local peer immediately sends getheaders again and then processes the headers
// - previous two steps repeat until a header within a week of earliestKeyTime is reached (further headers are ignored)
// - local peer sends getblocks
// - remote peer responds with inv containing up to 500 block hashes
// - local peer sends getdata with the block hashes
// - if there were 500 hashes, local peer sends getblocks again without waiting for remote peer
// - remote peer responds with multiple merkleblock and tx messages, followed by inv containing up to 500 block hashes
// - previous two steps repeat until an inv with fewer than 500 block hashes is received
// - local peer sends just getdata for the final set of fewer than 500 block hashes
// - remote peer responds with multiple merkleblock and tx messages
// - if at any point tx messages consume enough wallet addresses to drop below the bip32 chain gap limit, more addresses
//   are generated and local peer sends filterload with an updated bloom filter
// - after filterload is sent, getdata is sent to re-request recent blocks that may contain new tx matching the filter

typedef enum {
    inv_error = 0,
    inv_tx,
    inv_block,
    inv_merkleblock
} inv_type;

typedef struct {
    BRPeer peer; // superstruct on top of BRPeer
    char host[INET6_ADDRSTRLEN];
    BRPeerStatus status;
    int waitingForNetwork, needsFilterUpdate;
    uint64_t nonce;
    char *useragent;
    uint32_t version, lastblock, earliestKeyTime, currentBlockHeight;
    double startTime, pingTime;
    int sentVerack, gotVerack, sentGetaddr, sentFilter, sentGetdata, sentMempool, sentGetblocks;
    UInt256 *knownBlockHashes, *knownTxHashes;
    BRSet *knownTxHashSet, *currentBlockTxHashSet;
    BRMerkleBlock *currentBlock;
    int socket;
    void *info;
    void (*connected)(void *info);
    void (*disconnected)(void *info, int error);
    void (*relayedPeers)(void *info, const BRPeer peers[], size_t count);
    void (*relayedTx)(void *info, BRTransaction *tx);
    void (*hasTx)(void *info, UInt256 txHash);
    void (*rejectedTx)(void *info, UInt256 txHash, uint8_t code);
    void (*notfound)(void *info, const UInt256 txHashes[], size_t txCount, const UInt256 blockHashes[],
                     size_t blockCount);
    void (*relayedBlock)(void *info, BRMerkleBlock *block);
    void **pongInfo;
    void (**pongCallback)(void *info, int success); // yes, that's right, a pointer to a function pointer
    const BRTransaction *(*requestedTx)(void *info, UInt256 txHash);
    int (*networkIsReachable)(void *info);
    BRRWLock lock;
    pthread_t thread;
} BRPeerContext;

inline static int BRPeerIsIPv4(BRPeer *peer)
{
    return (peer->address.u64[0] == 0 && peer->address.u32[2] == be32(0xffff));
}

// returns a newly allocated BRPeer struct that must be freed by calling BRPeerFree()
BRPeer *BRPeerNew()
{
    BRPeerContext *ctx = calloc(1, sizeof(BRPeerContext));

    ctx->socket = -1;
    BRRWLockInit(&ctx->lock);
    return &ctx->peer;
}

static void BRPeerDidConnect(BRPeer *peer)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    
    if (ctx->status == BRPeerStatusConnecting && ctx->sentVerack && ctx->gotVerack) {
        peer_log(peer, "handshake completed");
        ctx->status = BRPeerStatusConnected;
        peer_log(peer, "connected with lastblock: %u", ctx->lastblock);
        if (ctx->connected) ctx->connected(ctx->info);
    }
}

static void BRPeerErrorDisconnect(BRPeer *peer, int error)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    
    // call shutdown() to causes the reader thread to exit before calling close() which releases the socket descriptor,
    // otherwise the descriptor can get immediately re-used, and any subsequent writes will result in file corruption
    if (ctx->socket >= 0) {
        shutdown(ctx->socket, SHUT_RDWR);
        BRRWLockWrite(&ctx->lock); // block until all socket writes are done
        if (ctx->socket >= 0) close(ctx->socket);
        ctx->socket = -1;
        BRRWLockUnlock(&ctx->lock);
        peer_log(peer, "disconnected");
        if (ctx->disconnected) ctx->disconnected(peer, error);
    }
}

static int BRPeerAcceptVersionMessage(BRPeer *peer, const uint8_t *msg, size_t len)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    size_t off = 0, strLen = 0, l = 0;
    uint64_t recvServices, fromServices, nonce;
    UInt128 recvAddr, fromAddr;
    uint16_t recvPort, fromPort;
    int r = 1;
    
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
    BRPeerContext *ctx = (BRPeerContext *)peer;
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
    BRPeerContext *ctx = (BRPeerContext *)peer;
    UInt256 hash;
    int r = 1;
    
    if (ctx->currentBlock && strncmp(MSG_TX, type, 12) != 0) { // if we receive a non-tx message, merkleblock is done
        hash = ctx->currentBlock->blockHash;
        ctx->currentBlock = NULL;
        BRSetClear(ctx->currentBlockTxHashSet);
        peer_log(peer, "incomplete merkleblock %s, expected %zu more tx, got %s", uint256_hex_encode(hash),
                 BRSetCount(ctx->currentBlockTxHashSet), type);
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
    struct sockaddr addr;
    struct timeval tv;
    fd_set fds;
    socklen_t addrLen, optLen;
    int socket = ((BRPeerContext *)peer)->socket;
    int arg = 0, count, error = 0, r = 1;

    if (socket >= 0) {
        arg = fcntl(socket, F_GETFL, NULL);
        if (arg < 0 || fcntl(socket, F_SETFL, arg | O_NONBLOCK) < 0) r = 0; // temporarily set the socket non-blocking
        if (! r) error = errno;
    }
    else r = 0;

    if (r) {
        memset(&addr, 0, sizeof(addr));
        
        if (BRPeerIsIPv4(peer)) {
            ((struct sockaddr_in *)&addr)->sin_family = AF_INET;
            ((struct sockaddr_in *)&addr)->sin_addr = *(struct in_addr *)&peer->address.u32[3];
            ((struct sockaddr_in *)&addr)->sin_port = htons(peer->port);
            addrLen = sizeof(struct sockaddr_in);
        }
        else {
            ((struct sockaddr_in6 *)&addr)->sin6_family = AF_INET6;
            ((struct sockaddr_in6 *)&addr)->sin6_addr = *(struct in6_addr *)&peer->address;
            ((struct sockaddr_in6 *)&addr)->sin6_port = htons(peer->port);
            addrLen = sizeof(struct sockaddr_in6);
        }

        if (connect(socket, &addr, addrLen) < 0) error = errno;

        if (error == EINPROGRESS) {
            error = 0;
            optLen = sizeof(error);
            tv.tv_sec = timeout;
            tv.tv_usec = (long)(timeout*1000000) % 1000000;
            FD_ZERO(&fds);
            FD_SET(socket, &fds);
            count = select(socket + 1, NULL, &fds, NULL, &tv);

            if (count <= 0 || getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &optLen) < 0 || error) {
                if (count == 0) error = ETIMEDOUT;
                if (count < 0 || ! error) error = errno;
                r = 0;
            }
        }

        if (r) peer_log(peer, "socket connected");
        fcntl(socket, F_SETFL, arg); // restore socket non-blocking status
    }

    if (! r && error) peer_log(peer, "connect error: %s", strerror(error));
    return r;
}

static void *BRPeerThreadRoutine(void *arg)
{
    BRPeer *peer = arg;
    BRPeerContext *ctx = arg;
    int error = 0;

    if (BRPeerOpenSocket(peer, CONNECT_TIMEOUT)) {
        uint8_t header[HEADER_LENGTH];
        size_t len = 0;
        ssize_t n = 0;

        ctx->startTime = (double)clock()/CLOCKS_PER_SEC;
        BRPeerSendVersionMessage(peer);
        
        while (! error && ctx->socket >= 0) {
            len = 0;

            while (! error && len < HEADER_LENGTH) {
                n = read(ctx->socket, header + len, sizeof(header) - len);
                if (n >= 0) len += n;
                if (n < 0 && errno != EWOULDBLOCK) error = errno;
                if (! error && ctx->status == BRPeerStatusConnecting &&
                    (double)clock()/CLOCKS_PER_SEC > ctx->startTime + CONNECT_TIMEOUT) error = ETIMEDOUT;
                    
                while (len >= sizeof(uint32_t) && *(uint32_t *)header != le32(MAGIC_NUMBER)) {
                    memmove(header, header + 1, --len); // consume one byte at a time until we find the magic number
                }
            }
            
            if (error) {
                peer_log(peer, "%s", strerror(error));
            }
            else if (header[15] != 0) { // verify header type field is NULL terminated
                peer_log(peer, "malformed message header: type not NULL terminated");
                error = EPROTO;
            }
            else if (len == HEADER_LENGTH) {
                const char *type = (const char *)(header + 4);
                uint32_t msgLen = le32(*(uint32_t *)(header + 16));
                uint32_t checksum = *(uint32_t *)(header + 20);
                
                if (msgLen > MAX_MSG_LENGTH) { // check message length
                    peer_log(peer, "error reading %s, message length %u is too long", type, msgLen);
                    error = EPROTO;
                }
                else {
                    uint8_t payload[msgLen];
                    UInt256 hash;
                    
                    len = 0;
                    
                    while (! error && len < msgLen) {
                        n = read(ctx->socket, payload + len, sizeof(payload) - len);
                        if (n >= 0) len += n;
                        if (n < 0 && errno != EWOULDBLOCK) error = errno;
                        if (! error && ctx->status == BRPeerStatusConnecting &&
                            (double)clock()/CLOCKS_PER_SEC > ctx->startTime + CONNECT_TIMEOUT) error = ETIMEDOUT;
                    }
                    
                    if (error) {
                        peer_log(peer, "%s", strerror(error));
                    }
                    else if (len == msgLen) {
                        BRSHA256_2(&hash, payload, msgLen);
                        
                        if (hash.u32[0] != checksum) { // verify checksum
                            peer_log(peer, "error reading %s, invalid checksum %x, expected %x, payload length:%u, "
                                     "SHA256_2:%s", type, be32(hash.u32[0]), be32(checksum), msgLen,
                                     uint256_hex_encode(hash));
                            error = EPROTO;
                        }
                        else if (! BRPeerAcceptMessage(peer, payload, msgLen, type)) error = EPROTO;
                    }
                }
            }
        }
    }

    BRPeerErrorDisconnect(peer, error);
    return NULL; // detached threads don't need to return a value
}

void BRPeerSetCallbacks(BRPeer *peer, void *info,
                        void (*connected)(void *info),
                        void (*disconnected)(void *info, int error),
                        void (*relayedPeers)(void *info, const BRPeer peers[], size_t count),
                        void (*relayedTx)(void *info, BRTransaction *tx),
                        void (*hasTx)(void *info, UInt256 txHash),
                        void (*rejectedTx)(void *info, UInt256 txHash, uint8_t code),
                        void (*relayedBlock)(void *info, BRMerkleBlock *block),
                        void (*notfound)(void *info, const UInt256 txHashes[], size_t txCount,
                                         const UInt256 blockHashes[], size_t blockCount),
                        const BRTransaction *(*requestedTx)(void *info, UInt256 txHash),
                        int (*networkIsReachable)(void *info))
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    
    ctx->info = info;
    ctx->connected = connected;
    ctx->disconnected = disconnected;
    ctx->relayedPeers = relayedPeers;
    ctx->relayedTx = relayedTx;
    ctx->hasTx = hasTx;
    ctx->rejectedTx = rejectedTx;
    ctx->relayedBlock = relayedBlock;
    ctx->notfound = notfound;
    ctx->requestedTx = requestedTx;
    ctx->networkIsReachable = networkIsReachable;
}

// set earliestKeyTime to wallet creation time in order to speed up initial sync
void BRPeerSetEarliestKeyTime(BRPeer *peer, uint32_t earliestKeyTime)
{
    ((BRPeerContext *)peer)->earliestKeyTime = earliestKeyTime;
}

// call this when local block height changes (helps detect tarpit nodes)
void BRPeerSetCurrentBlockHeight(BRPeer *peer, uint32_t currentBlockHeight)
{
    ((BRPeerContext *)peer)->currentBlockHeight = currentBlockHeight;
}

// current connection status
BRPeerStatus BRPeerConnectStatus(BRPeer *peer)
{
    return ((BRPeerContext *)peer)->status;
}

void BRPeerConnect(BRPeer *peer)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    struct timeval tv;
    int on = 1;
    pthread_attr_t attr;

    if (ctx->status == BRPeerStatusDisconnected || ctx->waitingForNetwork) {
        ctx->status = BRPeerStatusConnecting;
        ctx->pingTime = DBL_MAX;
    
        if (ctx->networkIsReachable && ! ctx->networkIsReachable(ctx->info)) { // delay until network is reachable
            ctx->waitingForNetwork = 1;
        }
        else {
            ctx->waitingForNetwork = 0;
            array_new(ctx->knownBlockHashes, 10);
            array_new(ctx->useragent, 40);
            array_new(ctx->knownTxHashes, 10);
            ctx->knownTxHashSet = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            ctx->currentBlockTxHashSet = BRSetNew(BRTransactionHash, BRTransactionEq, 10);
            array_new(ctx->pongInfo, 5);
            array_new(ctx->pongCallback, 5);
            ctx->socket = socket((BRPeerIsIPv4(peer) ? PF_INET : PF_INET6), SOCK_STREAM, 0);
            
            if (ctx->socket >= 0) {
                tv.tv_sec = 1; // one second timeout for send/receive, so thread doesn't block for too long
                tv.tv_usec = 0;
                setsockopt(ctx->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                setsockopt(ctx->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
                setsockopt(ctx->socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
#ifdef SO_NOSIGPIPE // BSD based systems have a SO_NOSIGPIPE socket option to supress SIGPIPE signals
                setsockopt(ctx->socket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
            }
            
            if (ctx->socket < 0 || pthread_attr_init(&attr) != 0) {
                ctx->status = BRPeerStatusDisconnected;
            }
            else if (pthread_attr_setstacksize(&attr, 512*1024) != 0 || // set stack size (there's no standard)
                     // set thread detached so it'll free resources immediately on exit without waiting for join
                     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
                     pthread_create(&ctx->thread, &attr, BRPeerThreadRoutine, peer) != 0) {
                peer_log(peer, "error creating thread");
                ctx->status = BRPeerStatusDisconnected;
                pthread_attr_destroy(&attr);
            }
        }
    }
}

void BRPeerDisconnect(BRPeer *peer)
{
    BRPeerErrorDisconnect(peer, 0); // disconnect with no error
}

// call this when wallet addresses need to be added to bloom filter
void BRPeerSetNeedsFilterUpdate(BRPeer *peer)
{
    ((BRPeerContext *)peer)->needsFilterUpdate = 1;
}

// display name of peer address
const char *BRPeerHost(BRPeer *peer)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;

    if (ctx->host[0] == '\0') {
        if (BRPeerIsIPv4(peer)) {
            inet_ntop(AF_INET, &peer->address.u32[3], ctx->host, sizeof(ctx->host));
        }
        else inet_ntop(AF_INET6, &peer->address, ctx->host, sizeof(ctx->host));
    }
    
    return ctx->host;
}

// connected peer version number
uint32_t BRPeerVersion(BRPeer *peer)
{
    return ((BRPeerContext *)peer)->version;
}

// connected peer user agent string
const char *BRPeerUserAgent(BRPeer *peer)
{
    return ((BRPeerContext *)peer)->useragent;
}

// best block height reported by connected peer
uint32_t BRPeerLastBlock(BRPeer *peer)
{
    return ((BRPeerContext *)peer)->lastblock;
}

// average ping time for connected peer
double BRPeerPingTime(BRPeer *peer)
{
    return ((BRPeerContext *)peer)->pingTime;
}

#ifndef MSG_NOSIGNAL   // linux based systems have a MSG_NOSIGNAL send flag, useful for supressing SIGPIPE signals
#define MSG_NOSIGNAL 0 // set to 0 if undefined (BSD has the SO_NOSIGPIPE sockopt, and windows has no signals at all)
#endif

// sends a bitcoin protocol message to peer
void BRPeerSendMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type)
{
    if (len > MAX_MSG_LENGTH) {
        peer_log(peer, "failed to send %s, length %zu is too long", type, len);
    }
    else {
        BRPeerContext *ctx = (BRPeerContext *)peer;
        uint8_t buf[HEADER_LENGTH + len], hash[32];
        size_t off = 0;
        ssize_t n = 0;
        int error = 0;
        
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
        
        if (ctx->socket >= 0) {
            BRRWLockRead(&ctx->lock); // obtain a read lock on peer to write to the socket
            len = 0;
            
            while (! error && len < sizeof(buf)) {
                n = send(ctx->socket, buf + len, sizeof(buf) - len, MSG_NOSIGNAL);
                if (n >= 0) len += n;
                if (n < 0 && errno != EWOULDBLOCK) error = errno;
                if (! error && ctx->status == BRPeerStatusConnecting &&
                    (double)clock()/CLOCKS_PER_SEC > ctx->startTime + CONNECT_TIMEOUT) error = ETIMEDOUT;
            }
            
            BRRWLockUnlock(&ctx->lock);
        }
        
        if (error) {
            peer_log(peer, "%s", strerror(error));
            BRPeerErrorDisconnect(peer, error);
        }
    }
}

void BRPeerSendVersionMessage(BRPeer *peer)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
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
    ctx->nonce = ((uint64_t)BRRand(0) << 32) | (uint64_t)BRRand(0); // random nonce
    *(uint64_t *)(msg + off) = le64(ctx->nonce);
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
    ((BRPeerContext *)peer)->sentVerack = 1;
}

void BRPeerSendFilterload(BRPeer *peer, const uint8_t *filter, size_t len)
{
    ((BRPeerContext *)peer)->sentFilter = 1;
    BRPeerSendMessage(peer, filter, len, MSG_FILTERLOAD);
}

void BRPeerSendMempool(BRPeer *peer)
{
    ((BRPeerContext *)peer)->sentMempool = 1;
    BRPeerSendMessage(peer, NULL, 0, MSG_MEMPOOL);
}

void BRPeerSendAddr(BRPeer *peer)
{
    uint8_t msg[BRVarIntSize(0)];
    size_t len = BRVarIntSet(msg, sizeof(msg), 0);
    
    //TODO: send peer addresses we know about
    BRPeerSendMessage(peer, msg, len, MSG_ADDR);
}

void BRPeerSendGetheaders(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop)
{
    size_t off = 0, len = sizeof(uint32_t) + BRVarIntSize(count) + sizeof(*locators)*count + sizeof(hashStop);
    uint8_t msg[len];
    
    if (off + sizeof(uint32_t) <= len) *(uint32_t *)(msg + off) = be32(PROTOCOL_VERSION);
    off += sizeof(uint32_t);
    if (off + BRVarIntSize(count) <= len) off += BRVarIntSet(msg + off, len - off, count);

    for (size_t i = 0; i < count; i++) {
        if (off + sizeof(*locators) <= len) *(UInt256 *)(msg + off) = locators[i];
        off += sizeof(*locators);
    }

    if (off + sizeof(hashStop) <= len) *(UInt256 *)(msg + off) = hashStop;
    off += sizeof(hashStop);

    if (off <= len && count > 0) {
        peer_log(peer, "calling getheaders with locators: [%s, %s]", uint256_hex_encode(locators[0]),
                 uint256_hex_encode(locators[count - 1]));
        BRPeerSendMessage(peer, msg, off, MSG_GETHEADERS);
    }
}

void BRPeerSendGetblocks(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop)
{
    size_t off = 0, len = sizeof(uint32_t) + BRVarIntSize(count) + sizeof(*locators)*count + sizeof(hashStop);
    uint8_t msg[len];
    
    if (off + sizeof(uint32_t) <= len) *(uint32_t *)(msg + off) = be32(PROTOCOL_VERSION);
    off += sizeof(uint32_t);
    if (off + BRVarIntSize(count) <= len) off += BRVarIntSet(msg + off, len - off, count);
    
    for (size_t i = 0; i < count; i++) {
        if (off + sizeof(*locators) <= len) *(UInt256 *)(msg + off) = locators[i];
        off += sizeof(*locators);
    }
    
    if (off + sizeof(hashStop) <= len) *(UInt256 *)(msg + off) = hashStop;
    off += sizeof(hashStop);
    
    if (off <= len && count > 0) {
        peer_log(peer, "calling getheaders with locators: [%s, %s]", uint256_hex_encode(locators[0]),
                 uint256_hex_encode(locators[count - 1]));
        BRPeerSendMessage(peer, msg, off, MSG_GETBLOCKS);
    }
}

void BRPeerSendInv(BRPeer *peer, const UInt256 txHashes[], size_t count)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    UInt256 *knownTxHashes = ctx->knownTxHashes;
    size_t i, j, hashesCount = array_count(knownTxHashes);

    for (i = 0; i < count; i++) {
        if (! BRSetContains(ctx->knownTxHashSet, &txHashes[i])) {
            array_add(knownTxHashes, txHashes[i]);

            if (ctx->knownTxHashes != knownTxHashes) { // check if knownTxHashes was moved to a new memory location
                ctx->knownTxHashes = knownTxHashes;
                BRSetClear(ctx->knownTxHashSet);
                for (j = array_count(knownTxHashes); j > 0; j--) BRSetAdd(ctx->knownTxHashSet, &knownTxHashes[j - 1]);
            }
            else BRSetAdd(ctx->knownTxHashSet, &array_last(knownTxHashes));
        }
    }
    
    count = array_count(knownTxHashes) - hashesCount;

    if (count > 0) {
        size_t off = 0, len = BRVarIntSize(count) + (sizeof(uint32_t) + sizeof(*txHashes))*count;
        uint8_t msg[len];
        
        if (off + BRVarIntSize(count) <= len) off += BRVarIntSet(msg + off, len - off, count);
        
        for (i = 0; i < count; i++) {
            if (off + sizeof(uint32_t) <= len) *(uint32_t *)(msg + off) = be32(inv_tx);
            off += sizeof(uint32_t);
            if (off + sizeof(*txHashes) <= len) *(UInt256 *)(msg + off) = knownTxHashes[hashesCount + i];
            off += sizeof(*txHashes);
        }

        if (off <= len) BRPeerSendMessage(peer, msg, off, MSG_INV);
    }
}

void BRPeerSendGetdata(BRPeer *peer, const UInt256 txHashes[], size_t txCount, const UInt256 blockHashes[],
                       size_t blockCount)
{
    size_t off = 0, count = txCount + blockCount;
    
    if (count > MAX_GETDATA_HASHES) { // limit total hash count to MAX_GETDATA_HASHES
        peer_log(peer, "couldn't send getdata, %zu is too many items, max is %d", count, MAX_GETDATA_HASHES);
    }
    else if (count > 0) {
        size_t len = BRVarIntSize(count) + (sizeof(uint32_t) + sizeof(UInt256))*(count);
        uint8_t msg[len];

        if (off + BRVarIntSize(count) <= len) off += BRVarIntSet(msg + off, len - off, count);
        
        for (size_t i = 0; i < txCount; i++) {
            if (off + sizeof(uint32_t) <= len) *(uint32_t *)(msg + off) = be32(inv_tx);
            off += sizeof(uint32_t);
            if (off + sizeof(*txHashes) <= len) *(UInt256 *)(msg + off) = txHashes[i];
            off += sizeof(*txHashes);
        }
        
        for (size_t i = 0; i < blockCount; i++) {
            if (off + sizeof(uint32_t) <= len) *(uint32_t *)(msg + off) = be32(inv_merkleblock);
            off += sizeof(uint32_t);
            if (off + sizeof(*blockHashes) <= len) *(UInt256 *)(msg + off) = blockHashes[i];
            off += sizeof(*blockHashes);
        }
        
        ((BRPeerContext *)peer)->sentGetdata = 1;
        BRPeerSendMessage(peer, msg, off, MSG_GETDATA);
    }
}

void BRPeerSendGetaddr(BRPeer *peer)
{
    ((BRPeerContext *)peer)->sentGetaddr = 1;
    BRPeerSendMessage(peer, NULL, 0, MSG_GETADDR);
}

void BRPeerSendPing(BRPeer *peer, void *info, void (*pongCallback)(void *info, int success))
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    uint64_t msg = be64(ctx->nonce);
    
    ctx->startTime = (double)clock()/CLOCKS_PER_SEC;
    array_add(ctx->pongInfo, info);
    array_add(ctx->pongCallback, pongCallback);
    BRPeerSendMessage(peer, (uint8_t *)&msg, sizeof(msg), MSG_PING);
}

// useful to get additional tx after a bloom filter update
void BRPeerRerequestBlocks(BRPeer *peer, UInt256 fromBlock)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    size_t i = array_count(ctx->knownBlockHashes);
    
    while (i > 0 && ! UInt256Eq(ctx->knownBlockHashes[i - 1], fromBlock)) i--;
   
    if (i > 0) {
        array_rm_range(ctx->knownBlockHashes, 0, i - 1);
        peer_log(peer, "re-requesting %zu blocks", array_count(ctx->knownBlockHashes));
        BRPeerSendGetdata(peer, NULL, 0, ctx->knownBlockHashes, array_count(ctx->knownBlockHashes));
    }
}

void BRPeerFree(BRPeer *peer)
{
    BRPeerContext *ctx = (BRPeerContext *)peer;
    
    BRRWLockWrite(&ctx->lock);
    if (ctx->useragent) array_free(ctx->useragent);
    if (ctx->knownBlockHashes) array_free(ctx->knownBlockHashes);
    if (ctx->knownTxHashes) array_free(ctx->knownTxHashes);
    if (ctx->knownTxHashSet) BRSetFree(ctx->knownTxHashSet);
    if (ctx->currentBlockTxHashSet) BRSetFree(ctx->currentBlockTxHashSet);
    if (ctx->pongInfo) array_free(ctx->pongInfo);
    if (ctx->pongCallback) array_free(ctx->pongCallback);
    BRRWLockUnlock(&ctx->lock);
    BRRWLockDestroy(&ctx->lock);
    free(ctx);
}
