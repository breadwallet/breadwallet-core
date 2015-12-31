//
//  BRPeer.h
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

#include "BRTransaction.h"
#include "BRMerkleBlock.h"
#include "BRInt.h"
#include <stdint.h>

#if BITCOIN_TESTNET
#define STANDARD_PORT 18333
#else
#define STANDARD_PORT 8333
#endif

#define SERVICES_NODE_NETWORK 1 // services value indicating a node carries full blocks, not just headers

#define BR_VERSION "0.6"
#define USER_AGENT "/breadwallet:" BR_VERSION "/"

// explanation of message types at: https://en.bitcoin.it/wiki/Protocol_specification
#define MSG_VERSION     "version"
#define MSG_VERACK      "verack"
#define MSG_ADDR        "addr"
#define MSG_INV         "inv"
#define MSG_GETDATA     "getdata"
#define MSG_NOTFOUND    "notfound"
#define MSG_GETBLOCKS   "getblocks"
#define MSG_GETHEADERS  "getheaders"
#define MSG_TX          "tx"
#define MSG_BLOCK       "block"
#define MSG_HEADERS     "headers"
#define MSG_GETADDR     "getaddr"
#define MSG_MEMPOOL     "mempool"
#define MSG_PING        "ping"
#define MSG_PONG        "pong"
#define MSG_FILTERLOAD  "filterload"
#define MSG_FILTERADD   "filteradd"
#define MSG_FILTERCLEAR "filterclear"
#define MSG_MERKLEBLOCK "merkleblock"
#define MSG_ALERT       "alert"
#define MSG_REJECT      "reject" //described in BIP61: https://github.com/bitcoin/bips/blob/master/bip-0061.mediawiki

typedef enum {
    BRPeerStatusDisconnected = 0,
    BRPeerStatusConnecting,
    BRPeerStatusConnected
} BRPeerStatus;

typedef struct {
    UInt128 address; // IPv6 address of peer
    uint16_t port; // port number for peer connection
    uint64_t services; // bitcoin network services supported by peer
    uint64_t timestamp; // timestamp reported by peer
    uint8_t flags; // scratch variable
    struct BRPeerContext *context;
} BRPeer;

#define BR_PEER_NONE ((BRPeer) { UINT128_ZERO, 0, 0, 0, 0, NULL })

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
                        const BRTransaction *(*requestedTx)(void *info, UInt256 txHash),
                        int (*networkIsReachable)(void *info));

BRPeerStatus BRPeerConnectStatus(BRPeer *peer); // current connection status
void BRPeerConnect(BRPeer *peer);
void BRPeerDisconnect(BRPeer *peer);

// set earliestKeyTime to wallet creation time in order to speed up initial sync
void BRPeerSetEarliestKeyTime(BRPeer *peer, uint32_t earliestKeyTime);

// call this when local best block height changes (helps detect tarpit nodes)
void BRPeerSetCurrentBlockHeight(BRPeer *peer, uint32_t currentBlockHeight);

// call this when wallet addresses need to be added to bloom filter
void BRPeerSetNeedsFilterUpdate(BRPeer *peer);

uint32_t BRPeerVersion(BRPeer *peer); // connected peer version number
const char *BRPeerUserAgent(BRPeer *peer); // connected peer user agent string
uint32_t BRPeerLastBlock(BRPeer *peer); // best block height reported by connected peer
double BRPeerPingTime(BRPeer *peer); // ping time for connected peer

void BRPeerSendMessage(BRPeer *peer, const uint8_t *msg, size_t len, const char *type);
void BRPeerSendVersionMessage(BRPeer *peer);
void BRPeerSendVerackMessage(BRPeer *peer);
void BRPeerSendFilterload(BRPeer *peer, const uint8_t *filter, size_t len);
void BRPeerSendMempool(BRPeer *peer);
void BRPeerSendGetheaders(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop);
void BRPeerSendGetblocks(BRPeer *peer, const UInt256 locators[], size_t count, UInt256 hashStop);
void BRPeerSendInv(BRPeer *peer, const UInt256 txHashes[], size_t count);
void BRPeerSendGetdata(BRPeer *peer, const UInt256 txHashes[], size_t txCount, const UInt256 blockHashes[],
                       size_t blockCount);
void BRPeerSendGetaddr(BRPeer *peer);
void BRPeerSendPing(BRPeer *peer, void *info, void (*pongCallback)(void *info, int success));
void BRPeerRerequestBlocks(BRPeer *peer, UInt256 fromBlock); // useful to get additional tx after a bloom filter update

inline static int BRPeerEq(const void *a, const void *b)
{
    return (UInt128Eq(((BRPeer *)a)->address, ((BRPeer *)b)->address) && ((BRPeer *)a)->port == ((BRPeer *)b)->port);
}

inline static size_t BRPeerHash(const void *peer)
{
    return (((BRPeer *)peer)->address.u32[3] ^ ((BRPeer *)peer)->port)*0x01000193; // (address xor port)*FNV_PRIME
}

#endif // BRPeer_h
