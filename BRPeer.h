//
//  BRPeer.h
//  breadwallet-core
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

#ifndef BRPeer_h
#define BRPeer_h

#include "BRTypes.h"
#include "BRTransaction.h"
#include "BRMerkleBlock.h"
#include <stdint.h>

#if BR_TESTNET
#define BR_STANDARD_PORT 18333
#else
#define BR_STANDARD_PORT 8333
#endif

#define BR_TIMEOUT_CODE  1001

#define BR_SERVICES_NODE_NETWORK 1 // services value indicating a node carries full blocks, not just headers

#define BR_USER_AGENT    "/breadwallet:0.6/"

// explanation of message types at: https://en.bitcoin.it/wiki/Protocol_specification
#define BR_MSG_VERSION     "version"
#define BR_MSG_VERACK      "verack"
#define BR_MSG_ADDR        "addr"
#define BR_MSG_INV         "inv"
#define BR_MSG_GETDATA     "getdata"
#define BR_MSG_NOTFOUND    "notfound"
#define BR_MSG_GETBLOCKS   "getblocks"
#define BR_MSG_GETHEADERS  "getheaders"
#define BR_MSG_TX          "tx"
#define BR_MSG_BLOCK       "block"
#define BR_MSG_HEADERS     "headers"
#define BR_MSG_GETADDR     "getaddr"
#define BR_MSG_MEMPOOL     "mempool"
#define BR_MSG_CHECKORDER  "checkorder"
#define BR_MSG_SUBMITORDER "submitorder"
#define BR_MSG_REPLY       "reply"
#define BR_MSG_PING        "ping"
#define BR_MSG_PONG        "pong"
#define BR_MSG_FILTERLOAD  "filterload"
#define BR_MSG_FILTERADD   "filteradd"
#define BR_MSG_FILTERCLEAR "filterclear"
#define BR_MSG_MERKLEBLOCK "merkleblock"
#define BR_MSG_ALERT       "alert"
#define BR_MSG_REJECT      "reject" //described in BIP61: https://github.com/bitcoin/bips/blob/master/bip-0061.mediawiki

typedef enum {
    BRPeerStatusDisconnected = 0,
    BRPeerStatusConnecting,
    BRPeerStatusConnected
} BRPeerStatus;

typedef enum {
    BRPeerErrorNone = 0
} BRPeerError;

typedef struct {
    UInt128 address;
    uint16_t port;
    uint64_t services;
    uint32_t timestamp; // last seen time
    uint8_t flags; // scratch variable
    struct BRPeerContext *context;
} BRPeer;

// set earliestKeyTime to the timestamp when the wallet was created to improve initial sync time
BRPeer *BRPeerCreate(void *(*alloc)(size_t), uint32_t earliestKeyTime);

void BRPeerSetCallbacks(BRPeer *peer,
                        void (*connected)(BRPeer *peer),
                        void (*disconnected)(BRPeer *peer, BRPeerError error),
                        void (*relayedPeers)(BRPeer *peer, BRPeer **peers, size_t count),
                        void (*relayedTx)(BRPeer *peer, BRTransaction *tx),
                        void (*hasTx)(BRPeer *peer, UInt256 txHash),
                        void (*rejectedTx)(BRPeer *peer, UInt256 txHash, uint8_t code),
                        void (*relayedBlock)(BRPeer *peer, BRMerkleBlock *block),
                        BRTransaction *(*reqeustedTx)(BRPeer *peer, UInt256 txHash));

BRPeerStatus BRPeerCurrentStatus(BRPeer *peer);
void BRPeerConnect(BRPeer *peer);
void BRPeerDisconnect(BRPeer *peer);

// call this when wallet addresses need to be added to bloom filter
void BRPeerNeedsFilterUpdate(BRPeer *peer);

// call this when local block height changes (helps detect tarpit nodes)
void BRPeerSetCurrentBlockHeight(BRPeer *peer, uint32_t currentBlockHeight);

uint32_t BRPeerVersion(BRPeer *peer);
const char *BRPeerUserAgent(BRPeer *peer);
uint32_t BRPeerLastBlock(BRPeer *peer);
double BRPeerPingTime(BRPeer *peer);

void BRPeerSendMessage(BRPeer *peer, const uint8_t *message, size_t len, const char *type);
void BRPeerSendFilterload(BRPeer *peer, const uint8_t *filter, size_t len);
void BRPeerSendMempool(BRPeer *peer);
void BRPeerSendGetheaders(BRPeer *peer, UInt256 *locators, size_t count, UInt256 hashStop);
void BRPeerSendGetblocks(BRPeer *peer, UInt256 *locators, size_t count, UInt256 hashStop);
void BRPeerSendInv(BRPeer *peer, UInt256 *txHashes, size_t count);
void BRPeerSendGetdata(BRPeer *peer, UInt256 *txHashes, size_t txCount, UInt256 *blockHashes, size_t blockCount);
void BRPeerSendGetaddr(BRPeer *peer);
void BRPeerSendPing(BRPeer *peer, void (*pong)(int success, void *info), void *info);
void BRPeerRerequestBlocks(BRPeer *peer, UInt256 fromBlock); // useful to get additional tx after a bloom filter update

void BRPeerFree(BRPeer *peer, void (*free)(void *));

#endif // BRPeer_h
