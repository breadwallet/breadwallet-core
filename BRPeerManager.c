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
#include "BRBloomFilter.h"
#include "BRSet.h"
#include "BRArray.h"
#include "BRInt.h"
#include "BRRWLock.h"
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <errno.h>
#include <netdb.h>

#define PROTOCOL_TIMEOUT     20.0
#define MAX_CONNECT_FAILURES 20 // notify user of network problems after this many connect failures in a row
#define CHECKPOINT_COUNT     (sizeof(checkpoint_array)/sizeof(*checkpoint_array))
#define GENESIS_BLOCK_HASH   (UInt256Reverse(uint256_hex_decode(checkpoint_array[0].hash)))
#define PEER_FLAG_SYNCED     0x01

#define str(x) _str(x)
#define _str(x) #x

#if BITCOIN_TESTNET

static const struct { uint32_t height; const char *hash; uint32_t timestamp; uint32_t target; } checkpoint_array[] = {
    {      0, "000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943", 1296688602, 0x1d00ffff },
    {  20160, "000000001cf5440e7c9ae69f655759b17a32aad141896defd55bb895b7cfc44e", 1345001466, 0x1c4d1756 },
    {  40320, "000000008011f56b8c92ff27fb502df5723171c5374673670ef0eee3696aee6d", 1355980158, 0x1d00ffff },
    {  60480, "00000000130f90cda6a43048a58788c0a5c75fa3c32d38f788458eb8f6952cee", 1363746033, 0x1c1eca8a },
    {  80640, "00000000002d0a8b51a9c028918db3068f976e3373d586f08201a4449619731c", 1369042673, 0x1c011c48 },
    { 100800, "0000000000a33112f86f3f7b0aa590cb4949b84c2d9c673e9e303257b3be9000", 1376543922, 0x1c00d907 },
    { 120960, "00000000003367e56e7f08fdd13b85bbb31c5bace2f8ca2b0000904d84960d0c", 1382025703, 0x1c00df4c },
    { 141120, "0000000007da2f551c3acd00e34cc389a4c6b6b3fad0e4e67907ad4c7ed6ab9f", 1384495076, 0x1c0ffff0 },
    { 161280, "0000000001d1b79a1aec5702aaa39bad593980dfe26799697085206ef9513486", 1388980370, 0x1c03fffc },
    { 181440, "00000000002bb4563a0ec21dc4136b37dcd1b9d577a75a695c8dd0b861e1307e", 1392304311, 0x1b336ce6 },
    { 201600, "0000000000376bb71314321c45de3015fe958543afcbada242a3b1b072498e38", 1393813869, 0x1b602ac0 }
};

static const char *dns_seeds[] = {
    "testnet-seed.breadwallet.com.", "testnet-seed.bitcoin.petertodd.org.", "testnet-seed.bluematt.me.",
    "testnet-seed.bitcoin.schildbach.de."
};

#else // main net

// blockchain checkpoints - these are also used as starting points for partial chain downloads, so they need to be at
// difficulty transition boundaries in order to verify the block difficulty at the immediately following transition
static const struct { uint32_t height; const char *hash; uint32_t timestamp; uint32_t target; } checkpoint_array[] = {
    {      0, "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f", 1231006505, 0x1d00ffff },
    {  20160, "000000000f1aef56190aee63d33a373e6487132d522ff4cd98ccfc96566d461e", 1248481816, 0x1d00ffff },
    {  40320, "0000000045861e169b5a961b7034f8de9e98022e7a39100dde3ae3ea240d7245", 1266191579, 0x1c654657 },
    {  60480, "000000000632e22ce73ed38f46d5b408ff1cff2cc9e10daaf437dfd655153837", 1276298786, 0x1c0eba64 },
    {  80640, "0000000000307c80b87edf9f6a0697e2f01db67e518c8a4d6065d1d859a3a659", 1284861847, 0x1b4766ed },
    { 100800, "000000000000e383d43cc471c64a9a4a46794026989ef4ff9611d5acb704e47a", 1294031411, 0x1b0404cb },
    { 120960, "0000000000002c920cf7e4406b969ae9c807b5c4f271f490ca3de1b0770836fc", 1304131980, 0x1b0098fa },
    { 141120, "00000000000002d214e1af085eda0a780a8446698ab5c0128b6392e189886114", 1313451894, 0x1a094a86 },
    { 161280, "00000000000005911fe26209de7ff510a8306475b75ceffd434b68dc31943b99", 1326047176, 0x1a0d69d7 },
    { 181440, "00000000000000e527fc19df0992d58c12b98ef5a17544696bbba67812ef0e64", 1337883029, 0x1a0a8b5f },
    { 201600, "00000000000003a5e28bef30ad31f1f9be706e91ae9dda54179a95c9f9cd9ad0", 1349226660, 0x1a057e08 },
    { 221760, "00000000000000fc85dd77ea5ed6020f9e333589392560b40908d3264bd1f401", 1361148470, 0x1a04985c },
    { 241920, "00000000000000b79f259ad14635739aaf0cc48875874b6aeecc7308267b50fa", 1371418654, 0x1a00de15 },
    { 262080, "000000000000000aa77be1c33deac6b8d3b7b0757d02ce72fffddc768235d0e2", 1381070552, 0x1916b0ca },
    { 282240, "0000000000000000ef9ee7529607286669763763e0c46acfdefd8a2306de5ca8", 1390570126, 0x1901f52c },
    { 302400, "0000000000000000472132c4daaf358acaf461ff1c3e96577a74e5ebf91bb170", 1400928750, 0x18692842 },
    { 322560, "000000000000000002df2dd9d4fe0578392e519610e341dd09025469f101cfa1", 1411680080, 0x181fb893 },
    { 342720, "00000000000000000f9cfece8494800d3dcbf9583232825da640c8703bcd27e7", 1423496415, 0x1818bb87 },
    { 362880, "000000000000000014898b8e6538392702ffb9450f904c80ebf9d82b519a77d5", 1435475246, 0x1816418e },
    { 383040, "00000000000000000a974fa1a3f84055ad5ef0b2f96328bc96310ce83da801c9", 1447236692, 0x1810b289 }
};

static const char *dns_seeds[] = {
    "seed.breadwallet.com.", "seed.bitcoin.sipa.be.", "dnsseed.bluematt.me.", "dnsseed.bitcoin.dashjr.org.",
    "seed.bitcoinstats.com.", "bitseed.xf2.org.", "seed.bitcoin.jonasschnelli.ch."
};

#endif

// comparator for sorting peers by timestamp, most recent first
inline static int BRPeerTimestampCompare(const void *peer, const void *otherPeer)
{
    if (((BRPeer *)peer)->timestamp < ((BRPeer *)otherPeer)->timestamp) return 1;
    if (((BRPeer *)peer)->timestamp > ((BRPeer *)otherPeer)->timestamp) return -1;
    return 0;
}

// returns a hash value for a block's prevBlock value suitable for use in a hashtable
inline static size_t BRPrevBlockHash(const void *block)
{
    return *(const size_t *)&((const BRMerkleBlock *)block)->prevBlock;
}

// true if block and otherBlock have equal prevBlock values
inline static int BRPrevBlockEq(const void *block, const void *otherBlock)
{
    return UInt256Eq(((const BRMerkleBlock *)block)->prevBlock, ((const BRMerkleBlock *)otherBlock)->prevBlock);
}

// returns a hash value for a block's height value suitable for use in a hashtable
inline static size_t BRBlockHeightHash(const void *block)
{
    return (size_t)((const BRMerkleBlock *)block)->height;
}

// true if block and otherBlock have equal height values
inline static int BRBlockHeightEq(const void *block, const void *otherBlock)
{
    return (((const BRMerkleBlock *)block)->height == ((const BRMerkleBlock *)otherBlock)->height);
}

struct _BRPeerManager {
    BRWallet *wallet;
    int connected, connectFailures;
    BRPeer *peers, *downloadPeer, **connectedPeers;
    uint32_t tweak, earliestKeyTime, syncStartHeight, filterUpdateHeight;
    BRBloomFilter *bloomFilter;
    double fpRate, lastRelayTime;
    BRSet *blocks, *orphans, *checkpoints;
    BRMerkleBlock *lastBlock, *lastOrphan;
    UInt256 *nonFpTx;
    struct { UInt256 txHash; BRPeer *peers; } *txRelays;
    BRTransaction **publishedTx;
    UInt256 *publishedHash;
    void **publishedInfo;
    void (*publishedCallback)(void *info, int error);
    void *callbackInfo;
    void (*syncStarted)(void *info);
    void (*syncSucceded)(void *info);
    void (*syncFailed)(void *info, int error);
    void (*txStatusUpdate)(void *info);
    void (*txRejected)(void *info, int rescanRecommended);
    void (*saveBlocks)(void *info, BRMerkleBlock *blocks[], size_t count);
    void (*savePeers)(void *info, const BRPeer peers[], size_t count);
    int (*networkIsReachable)(void *info);
    BRRWLock lock;
};

typedef struct {
    BRPeer *peer;
    BRPeerManager *manager;
} BRPeerCallbackInfo;

static void BRPeerManagerPeerMisbehavin(BRPeerManager *manager, BRPeer *peer)
{
}

static void BRPeerManagerSyncStopped(BRPeerManager *manager)
{
}

// adds transaction to list of tx to be published, along with any unconfirmed inputs
static void BRPeerManagerAddTxToPublishList(BRPeerManager *manager, BRTransaction *tx)
{
    if (tx && tx->blockHeight == TX_UNCONFIRMED) {
        for (size_t i = array_count(manager->publishedTx); i > 0; i--) {
            if (BRTransactionEq(manager->publishedTx[i - 1], tx)) return;
        }
        
        array_add(manager->publishedTx, tx);
        array_add(manager->publishedHash, tx->txHash);

        for (size_t i = 0; i < tx->inCount; i++) {
            BRPeerManagerAddTxToPublishList(manager, BRWalletTransactionForHash(manager->wallet, tx->inputs[i].txHash));
        }
    }
}

static void BRPeerManagerLoadBloomFilter(BRPeerManager *manager, BRPeer *peer)
{
    BRBloomFilter *filter = manager->bloomFilter;
    
    if (! filter) {
        // every time a new wallet address is added, the bloom filter has to be rebuilt, and each address is only used
        // for one transaction, so here we generate some spare addresses to avoid rebuilding the filter each time a
        // wallet transaction is encountered during the chain sync
        BRWalletUnusedAddrs(manager->wallet, NULL, SEQUENCE_GAP_LIMIT_EXTERNAL + 100, 0);
        BRWalletUnusedAddrs(manager->wallet, NULL, SEQUENCE_GAP_LIMIT_INTERNAL + 100, 1);

        BRSetClear(manager->orphans); // clear out orphans that may have been received on an old filter
        manager->lastOrphan = NULL;
        manager->filterUpdateHeight = manager->lastBlock->height;
        manager->fpRate = BLOOM_REDUCED_FALSEPOSITIVE_RATE;

        BRAddress addrs[BRWalletAllAddrs(manager->wallet, NULL, 0)];
        size_t addrsCount = BRWalletAllAddrs(manager->wallet, addrs, sizeof(addrs)/sizeof(*addrs));
        BRUTXO utxos[BRWalletUTXOs(manager->wallet, NULL, 0)];
        size_t utxosCount = BRWalletUTXOs(manager->wallet, utxos, sizeof(utxos)/sizeof(*utxos));
        BRBloomFilter *filter = BRBloomFilterNew(manager->fpRate, addrsCount + utxosCount + 100, manager->tweak,
                                                 BLOOM_UPDATE_ALL);
        
        for (size_t i = 0; i < addrsCount; i++) { // add addresses to watch for tx receiveing money to the wallet
            UInt160 hash = UINT160_ZERO;
            
            BRAddressHash160(&hash, addrs[i].s);
        
            if (! UInt160IsZero(hash) && ! BRBloomFilterContainsData(filter, hash.u8, sizeof(hash))) {
                BRBloomFilterInsertData(filter, hash.u8, sizeof(hash));
            }
        }

        for (size_t i = 0; i < utxosCount; i++) { // add UTXOs to watch for tx sending money from the wallet
            uint8_t o[sizeof(UInt256) + sizeof(uint32_t)];

            *(UInt256 *)o = utxos[i].hash;
            *(uint32_t *)(o + sizeof(UInt256)) = le32(utxos[i].n);
            if (! BRBloomFilterContainsData(filter, o, sizeof(o))) BRBloomFilterInsertData(filter, o, sizeof(o));
        }
    
        // TODO: XXX if already synced, recursively add inputs of unconfirmed receives
        manager->bloomFilter = filter;
    }

    uint8_t data[BRBloomFilterSerialize(filter, NULL, 0)];
    size_t len = BRBloomFilterSerialize(filter, data, sizeof(data));
    
    BRPeerSendFilterload(peer, data, len);
}

// unconfirmed transactions that aren't in the mempools of any of connected peers have likely dropped off the network
static void BRPeerManagerRmUnrelayedTx(BRPeerManager *manager)
{
}

static void BRPeerManagerLoadMempools(BRPeerManager *manager)
{
}

static size_t BRPeerManagerBlockLocators(BRPeerManager *manager, UInt256 locators[], size_t count)
{
    // append 10 most recent block hashes, decending, then continue appending, doubling the step back each time,
    // finishing with the genesis block (top, -1, -2, -3, -4, -5, -6, -7, -8, -9, -11, -15, -23, -39, -71, -135, ..., 0)
    BRMerkleBlock *block = manager->lastBlock;
    int32_t step = 1, i = 0, j;
    
    while (block && block->height > 0) {
        if (locators && i < count) locators[i] = block->blockHash;
        if (++i >= 10) step *= 2;
        
        for (j = 0; block && j < step; j++) {
            block = BRSetGet(manager->blocks, &block->prevBlock);
        }
    }

    if (locators && i < count) locators[i] = GENESIS_BLOCK_HASH;
    return ++i;
}

// returns a UINT128_ZERO terminated array of addresses for hostname that must be freed, or NULL on lookup failure
static UInt128 *addressLookup(const char *hostname)
{
    struct addrinfo *servinfo, *p;
    UInt128 *addrList = NULL;
    size_t count = 0, i = 0;
    
    if (getaddrinfo(hostname, NULL, NULL, &servinfo) == 0) {
        for (p = servinfo; p != NULL; p = p->ai_next) count++;
        if (count > 0) addrList = calloc(count + 1, sizeof(*addrList));
        
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if (p->ai_family == AF_INET) {
                addrList[i].u32[2] = be32(0xffff);
                addrList[i].u32[3] = ((struct sockaddr_in *)p->ai_addr)->sin_addr.s_addr;
                i++;
            }
//            else if (p->ai_family == AF_INET6) {
//                addrList[i++] = *(UInt128 *)&((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
//            }
        }
        
        freeaddrinfo(servinfo);
    }
    
    return addrList;
}

// DNS peer discovery
static void BRPeerManagerFindPeers(BRPeerManager *manager)
{
}

static void peerConnectedMempoolDone(void *info, int success)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;

    if (success) {
        peer->flags |= PEER_FLAG_SYNCED;
        BRPeerSendGetaddr(peer); // request a list of other bitcoin peers
        BRPeerManagerRmUnrelayedTx(manager);
    }
}

static void peerConnectedFilterloadDone(void *info, int success)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;

    BRPeerSendMempool(peer);
    BRPeerSendPing(peer, info, peerConnectedMempoolDone);
}

static void peerConnected(void *info)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
    time_t now = time(NULL);
    
    if (peer->timestamp > now + 2*60*60 || peer->timestamp < now - 2*60*60) peer->timestamp = now; // sanity check
    manager->connectFailures = 0;
    
    // drop peers that don't carry full blocks, or aren't synced yet
    // TODO: XXXX does this work with 0.11 pruned nodes?
    if (! (peer->services & SERVICES_NODE_NETWORK) ||
        BRPeerLastBlock(peer) + 10 < manager->lastBlock->height) {
        BRPeerDisconnect(peer);
    }
    else {
        BRBloomFilter *filter;
        BRTransaction *unconfirmed[BRWalletUnconfirmedTx(manager->wallet, NULL, 0)];
        size_t unconfirmedCount = BRWalletUnconfirmedTx(manager->wallet, unconfirmed,
                                                        sizeof(unconfirmed)/sizeof(*unconfirmed));

        for (size_t i = 0; i < unconfirmedCount; i++) {
            if (BRWalletAmountSentByTx(manager->wallet, unconfirmed[i]) > 0 &&
                BRWalletTransactionIsValid(manager->wallet, unconfirmed[i])) {
                BRPeerManagerAddTxToPublishList(manager, unconfirmed[i]); // add unconfirmed valid send tx to mempool
            }
        }
    
        // check if we should stick with the existing download peer
        if (manager->connected && (BRPeerLastBlock(manager->downloadPeer) >= BRPeerLastBlock(peer) ||
                                   manager->lastBlock->height >= BRPeerLastBlock(peer))) {
            // only load bloom filter if we're done syncing
            if (manager->lastBlock->height >= BRPeerLastBlock(peer)) {
                BRPeerManagerLoadBloomFilter(manager, peer);
                BRPeerSendInv(peer, manager->publishedHash, array_count(manager->publishedHash)); // publish pending tx
                BRPeerSendPing(peer, info, peerConnectedFilterloadDone);
            }
        }
        else {
            // select the peer with the lowest ping time to download the chain from if we're behind
            // BUG: XXX a malicious peer can report a higher lastblock to make us select them as the download peer, if
            // two peers agree on lastblock, use one of those two instead
            for (size_t i = array_count(manager->connectedPeers); i > 0; i--) {
                BRPeer *p = manager->connectedPeers[i - 1];

                if ((BRPeerPingTime(p) < BRPeerPingTime(peer) && BRPeerLastBlock(p) >= BRPeerLastBlock(peer)) ||
                    BRPeerLastBlock(p) > BRPeerLastBlock(peer)) peer = p;
            }

            if (manager->downloadPeer) BRPeerDisconnect(manager->downloadPeer);
            manager->downloadPeer = peer;
            manager->connected = 1;
            filter = manager->bloomFilter;
            manager->bloomFilter = NULL; // make sure the bloom filter is updated with any newly generated addresses
            if (filter) BRBloomFilterFree(filter);
            BRPeerManagerLoadBloomFilter(manager, peer);
            BRPeerSetCurrentBlockHeight(peer, manager->lastBlock->height);
            BRPeerSendInv(peer, manager->publishedHash, array_count(manager->publishedHash)); // publish pending tx

            if (manager->lastBlock->height < BRPeerLastBlock(peer)) { // start blockchain sync
                UInt256 locators[BRPeerManagerBlockLocators(manager, NULL, 0)];
                size_t count = BRPeerManagerBlockLocators(manager, locators, sizeof(locators)/sizeof(*locators));
            
                manager->lastRelayTime = now;
                // TODO: XXX implement sync stall timer

                // request just block headers up to a week before earliestKeyTime, and then merkleblocks after that
                // BUG: XXX headers can timeout on slow connections (each message is over 160k)
                if (manager->lastBlock->timestamp + 7*24*60*60 >= manager->earliestKeyTime) {
                    BRPeerSendGetblocks(peer, locators, count, UINT256_ZERO);
                }
                else BRPeerSendGetheaders(peer, locators, count, UINT256_ZERO);
            }
            else BRPeerManagerLoadMempools(manager); // we're already synced
        }
    }
}

static void peerDisconnected(void *info, int error)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
    
    if (error == EPROTO) { // if it's protocol error, the peer isn't following standard policy
        BRPeerManagerPeerMisbehavin(manager, peer);
    }
    else if (error) { // timeout or some non-protocol related network error
        for (size_t i = array_count(manager->peers); i > 0; i--) {
            if (BRPeerEq(&manager->peers[i - 1], peer)) array_rm(manager->peers, i - 1);
        }
        
        manager->connectFailures++;
    }
    
    for (size_t i = array_count(manager->txRelays); i > 0; i--) {
        for (size_t j = array_count(manager->txRelays[i - 1].peers); j > 0; j--) {
            if (BRPeerEq(&manager->txRelays[i - 1].peers[j - 1], peer)) array_rm(manager->txRelays[i - 1].peers, j - 1);
        }
    }

    if (BRPeerEq(manager->downloadPeer, peer)) { // download peer disconnected
        manager->connected = 0;
        manager->downloadPeer = NULL;
        if (manager->connectFailures > MAX_CONNECT_FAILURES) manager->connectFailures = MAX_CONNECT_FAILURES;
    }

    if (! manager->connected && manager->connectFailures == MAX_CONNECT_FAILURES) {
        BRPeerManagerSyncStopped(manager);
        
        // clear out stored peers so we get a fresh list from DNS on next connect attempt
        array_clear(manager->peers);
        if (manager->savePeers) manager->savePeers(manager->callbackInfo, NULL, 0);
        if (manager->syncFailed) manager->syncFailed(manager->callbackInfo, error);
    }
    else if (manager->connectFailures < MAX_CONNECT_FAILURES) {
        BRPeerManagerConnect(manager); // try connecting to another peer
    }
    
    if (manager->txStatusUpdate) manager->txStatusUpdate(manager->callbackInfo);
    free(info);
}

static void peerRelayedPeers(void *info, const BRPeer peers[], size_t count)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static void peerRelayedTx(void *info, BRTransaction *tx)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static void peerHasTx(void *info, UInt256 txHash)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static void peerRejectedTx(void *info, UInt256 txHash, uint8_t code)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static void peerRelayedBlock(void *info, BRMerkleBlock *block)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static void peerDataNotfound(void *info, const UInt256 txHashes[], size_t txCount,
                             const UInt256 blockHashes[], size_t blockCount)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;
}

static const BRTransaction *peerRequestedTx(void *info, UInt256 txHash)
{
    BRPeer *peer = ((BRPeerCallbackInfo *)info)->peer;
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;

    return NULL;
}

static int peerNetworkIsReachable(void *info)
{
    BRPeerManager *manager = ((BRPeerCallbackInfo *)info)->manager;

    return (manager->networkIsReachable) ? manager->networkIsReachable(manager->callbackInfo) : 1;
}

// returns a newly allocated BRPeerManager struct that must be freed by calling BRPeerManagerFree()
BRPeerManager *BRPeerManagerNew(BRWallet *wallet, uint32_t earliestKeyTime, BRMerkleBlock *blocks[], size_t blocksCount,
                                const BRPeer peers[], size_t peersCount)
{
    BRPeerManager *manager = calloc(1, sizeof(BRPeerManager));
    
    manager->wallet = wallet;
    manager->earliestKeyTime = earliestKeyTime;
    manager->tweak = BRRand(BR_RAND_MAX);
    array_new(manager->peers, peersCount);
    if (peers) array_add_array(manager->peers, peers, peersCount);
    qsort(manager->peers, array_count(manager->peers), sizeof(*manager->peers), BRPeerTimestampCompare);
    array_new(manager->connectedPeers, PEER_MAX_CONNECTIONS);
    manager->blocks = BRSetNew(BRMerkleBlockHash, BRMerkleBlockEq, blocksCount);
    manager->orphans = BRSetNew(BRPrevBlockHash, BRPrevBlockEq, 10); // orphans are indexed by prevBlock
    manager->checkpoints = BRSetNew(BRBlockHeightHash, BRBlockHeightEq, 20); // checkpoints are indexed by height

    for (size_t i = 0; i < CHECKPOINT_COUNT; i++) {
        BRMerkleBlock *block = BRMerkleBlockNew();
        
        block->height = checkpoint_array[i].height;
        block->blockHash = UInt256Reverse(uint256_hex_decode(checkpoint_array[i].hash));
        block->timestamp = checkpoint_array[i].timestamp;
        block->target = checkpoint_array[i].target;
        BRSetAdd(manager->checkpoints, block);
        BRSetAdd(manager->blocks, block);
    }

    for (size_t i = 0; i < blocksCount; i++) BRSetAdd(manager->blocks, blocks[i]);
    array_new(manager->nonFpTx, 10);
    array_new(manager->txRelays, 10);
    array_new(manager->publishedTx, 10);
    array_new(manager->publishedHash, 10);
    array_new(manager->publishedInfo, 10);
    BRRWLockInit(&manager->lock);
    return manager;
}

void BRPeerManagerSetCallbacks(BRPeerManager *manager, void *info,
                               void (*syncStarted)(void *info),
                               void (*syncSucceded)(void *info),
                               void (*syncFailed)(void *info, int error),
                               void (*txStatusUpdate)(void *info),
                               void (*txRejected)(void *info, int rescanRecommended),
                               void (*saveBlocks)(void *info, BRMerkleBlock *blocks[], size_t count),
                               void (*savePeers)(void *info, const BRPeer peers[], size_t count),
                               int (*networkIsReachable)(void *info))
{
    manager->callbackInfo = info;
    manager->syncStarted = syncStarted;
    manager->syncSucceded = syncSucceded;
    manager->syncFailed = syncFailed;
    manager->txStatusUpdate = txStatusUpdate;
    manager->txRejected = txRejected;
    manager->saveBlocks = saveBlocks;
    manager->savePeers = savePeers;
    manager->networkIsReachable = networkIsReachable;
}

// true if currently connected to at least one peer
int BRPeerMangerIsConnected(BRPeerManager *manager)
{
    return manager->connected;
}

// connect to bitcoin peer-to-peer network (also call this whenever networkIsReachable() status changes)
void BRPeerManagerConnect(BRPeerManager *manager)
{
#if ! defined(MSG_NOSIGNAL) && ! defined(SO_NOSIGPIPE)
    // TODO: XXX disable SIGPIPE for process
#endif

    if (manager->connectFailures >= MAX_CONNECT_FAILURES) manager->connectFailures = 0; // this is a manual retry
    
    if (BRPeerManagerSyncProgress(manager) < 1.0 - DBL_EPSILON) {
        if (manager->syncStartHeight == 0) manager->syncStartHeight = manager->lastBlock->height;
        if (manager->syncStarted) manager->syncStarted(manager->callbackInfo);

        for (size_t i = array_count(manager->connectedPeers); i > 0; i--) {
            BRPeer *p = manager->connectedPeers[i - 1];

            if (BRPeerConnectStatus(p) == BRPeerStatusDisconnected) {
                array_rm(manager->connectedPeers, i - 1);
                BRPeerFree(p);
            }
        }
    
        if (array_count(manager->connectedPeers) < PEER_MAX_CONNECTIONS) {
            time_t now = time(NULL);
            BRPeer *peers;

            if (array_count(manager->peers) < PEER_MAX_CONNECTIONS ||
                manager->peers[PEER_MAX_CONNECTIONS - 1].timestamp + 3*24*60*60 < now) {
                BRPeerManagerFindPeers(manager);
            }
            
            array_new(peers, 100);
            array_add_array(peers, manager->peers,
                            (array_count(manager->peers) < 100) ? array_count(manager->peers) : 100);

            while (array_count(peers) > 0 && array_count(manager->connectedPeers) < PEER_MAX_CONNECTIONS) {
                // pick a random peer biased towards peers with more recent timestamps
                size_t i = pow(BRRand((uint32_t)array_count(peers)), 2)/array_count(peers);
                BRPeerCallbackInfo *info;
        
                for (size_t j = array_count(manager->connectedPeers); i != SIZE_MAX && j > 0; j--) {
                    if (! BRPeerEq(&peers[i], manager->connectedPeers[j - 1])) continue;
                    array_rm(peers, i); // already in connectedPeers
                    i = SIZE_MAX;
                }
                
                if (i != SIZE_MAX) {
                    info = calloc(1, sizeof(*info));
                    info->manager = manager;
                    info->peer = BRPeerNew();
                    *info->peer = peers[i];
                    array_rm(peers, i);
                    array_add(manager->connectedPeers, info->peer);
                    BRPeerSetCallbacks(info->peer, info, peerConnected, peerDisconnected, peerRelayedPeers,
                                       peerRelayedTx, peerHasTx, peerRejectedTx, peerRelayedBlock, peerDataNotfound,
                                       peerRequestedTx, peerNetworkIsReachable);
                    BRPeerSetEarliestKeyTime(info->peer, manager->earliestKeyTime);
                    BRPeerConnect(info->peer);
                }
            }

            if (array_count(manager->connectedPeers) == 0) {
                BRPeerManagerSyncStopped(manager);
                if (manager->syncFailed) manager->syncFailed(manager->callbackInfo, ENETUNREACH);
            }
        }
    }
}

// rescan blockchain for potentially missing transactions
void BRPeerManagerRescan(BRPeerManager *manager)
{
}

// current proof-of-work verified best block height
uint32_t BRPeerManagerLastBlockHeight(BRPeerManager *manager)
{
    return manager->lastBlock->height;
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
void BRPeerManagerPublishTx(BRPeerManager *manager, BRTransaction *tx, void *info,
                            void (*callback)(void *info, int error))
{
}

// number of connected peers that have relayed the transaction
size_t BRPeerMangaerRelayCount(BRPeerManager *manager, UInt256 txHash)
{
    return 0;
}

void setMapFreeBlock(void *info, void *block)
{
    BRMerkleBlockFree(block);
}

// frees memory allocated for manager
void BRPeerManagerFree(BRPeerManager *manager)
{
    BRRWLockWrite(&manager->lock);
    array_free(manager->peers);
    for (size_t i = array_count(manager->connectedPeers); i > 0; i--) BRPeerFree(manager->connectedPeers[i - 1]);
    array_free(manager->connectedPeers);
    BRSetMap(manager->blocks, NULL, setMapFreeBlock);
    BRSetFree(manager->blocks);
    BRSetFree(manager->orphans);
    BRSetFree(manager->checkpoints);
    array_free(manager->nonFpTx);
    array_free(manager->txRelays);
    array_free(manager->publishedTx);
    array_free(manager->publishedHash);
    array_free(manager->publishedInfo);
    BRRWLockUnlock(&manager->lock);
    BRRWLockDestroy(&manager->lock);
    free(manager);
}
