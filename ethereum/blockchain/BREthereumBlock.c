//
//  BBREthereumBlock.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/23/2018.
//  Copyright (c) 2018 breadwallet LLC
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BRArray.h"
#include "BREthereumBlock.h"
#include "BREthereumLog.h"
#include "../BREthereumPrivate.h"

// GETH:
/*
 type Header struct {
    ParentHash  common.Hash    `json:"parentHash"       gencodec:"required"`
    UncleHash   common.Hash    `json:"sha3Uncles"       gencodec:"required"`
    Coinbase    common.Address `json:"miner"            gencodec:"required"`
    Root        common.Hash    `json:"stateRoot"        gencodec:"required"`
    TxHash      common.Hash    `json:"transactionsRoot" gencodec:"required"`
    ReceiptHash common.Hash    `json:"receiptsRoot"     gencodec:"required"`
    Bloom       Bloom          `json:"logsBloom"        gencodec:"required"`
    Difficulty  *big.Int       `json:"difficulty"       gencodec:"required"`
    Number      *big.Int       `json:"number"           gencodec:"required"`
    GasLimit    uint64         `json:"gasLimit"         gencodec:"required"`
    GasUsed     uint64         `json:"gasUsed"          gencodec:"required"`
    Time        *big.Int       `json:"timestamp"        gencodec:"required"`
    Extra       []byte         `json:"extraData"        gencodec:"required"`
    MixDigest   common.Hash    `json:"mixHash"          gencodec:"required"`
    Nonce       BlockNonce     `json:"nonce"            gencodec:"required"`
}

// HashNoNonce returns the hash which is used as input for the proof-of-work search.
func (h *Header) HashNoNonce() common.Hash {
    return rlpHash([]interface{}{
        h.ParentHash,
        h.UncleHash,
        h.Coinbase,
        h.Root,
        h.TxHash,
        h.ReceiptHash,
        h.Bloom,
        h.Difficulty,
        h.Number,
        h.GasLimit,
        h.GasUsed,
        h.Time,
        h.Extra,
    })
}
*/
/**
 * An Ethereum Block header.
 *
 * As per (2018-05-04 https://ethereum.github.io/yellowpaper/paper.pdf). [Documentation from
 * that reference]
 */
struct BREthereumBlockHeaderRecord {
    // THIS MUST BE FIRST to support BRSet operations.
    //
    // The Keccak256-bit hash (of this Block).
    BREthereumHash hash;

    // The Keccak256-bit hash of the parent block’s header, in its entirety; formally Hp.
    BREthereumHash parentHash;

    // The Keccak 256-bit hash of the om- mers list portion of this block; formally Ho.
    BREthereumHash ommersHash;

    // The 160-bit address to which all fees collected from the successful mining of this block
    // be transferred; formally Hc.
    BREthereumAddress beneficiary;

    // The Keccak 256-bit hash of the root node of the state trie, after all transactions are
    // executed and finalisations applied; formally Hr.
    BREthereumHash stateRoot;

    // The Keccak 256-bit hash of the root node of the trie structure populated with each
    // transaction in the transactions list portion of the block; formally Ht.
    BREthereumHash transactionsRoot;

    // The Keccak 256-bit hash of the root node of the trie structure populated with the receipts
    // of each transaction in the transactions list portion of the block; formally He.
    BREthereumHash receiptsRoot;

    // The Bloom filter composed from indexable information (logger address and log topics)
    // contained in each log entry from the receipt of each transaction in the transactions list;
    // formally Hb.
    BREthereumBloomFilter logsBloom;

    // A scalar value corresponding to the difficulty level of this block. This can be calculated
    // from the previous block’s difficulty level and the timestamp; formally Hd.
    uint64_t difficulty;

    // A scalar value equal to the number of ancestor blocks. The genesis block has a number of
    // zero; formally Hi.
    uint64_t number;

    // A scalar value equal to the current limit of gas expenditure per block; formally Hl.
    uint64_t gasLimit;

    // A scalar value equal to the total gas used in transactions in this block; formally Hg.
    uint64_t gasUsed;

    // A scalar value equal to the reasonable output of Unix’s time() at this block’s inception;
    // formally Hs.
    uint64_t timestamp;

    // An arbitrary byte array containing data relevant to this block. This must be 32 bytes or
    // fewer; formally Hx.
    uint8_t extraData [32];
    uint8_t extraDataCount;

    // A 256-bit hash which, combined with the nonce, proves that a sufficient amount of
    // computation has been carried out on this block; formally Hm.
    BREthereumHash mixHash;

    // A 64-bitvaluewhich, combined with the mixHash, proves that a sufficient amount of
    // computation has been carried out on this block; formally Hn.
    uint64_t nonce;
};

static BREthereumBlockHeader
createBlockHeaderMinimal (BREthereumHash hash, uint64_t number, uint64_t timestamp) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof (struct BREthereumBlockHeaderRecord));
    header->number = number;
    header->timestamp = timestamp;
    header->hash = hash;
    return header;
}

static BREthereumBlockHeader
createBlockHeader (void) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof (struct BREthereumBlockHeaderRecord));

    //         EMPTY_DATA_HASH = sha3(EMPTY_BYTE_ARRAY);
    //         EMPTY_LIST_HASH = sha3(RLP.encodeList());

    // transactionRoot, receiptsRoot
    //         EMPTY_TRIE_HASH = sha3(RLP.encodeElement(EMPTY_BYTE_ARRAY));
    return header;
}

static void
blockHeaderFree (BREthereumBlockHeader header) {
    free (header);
}

extern BREthereumBoolean
blockHeaderIsValid (BREthereumBlockHeader header) {
    return ETHEREUM_BOOLEAN_TRUE;
}

extern BREthereumHash
blockHeaderGetHash (BREthereumBlockHeader header) {
    return header->hash;
}

extern BREthereumHash
blockHeaderGetParentHash (BREthereumBlockHeader header) {
    return header->parentHash;
}

extern uint64_t
blockHeaderGetNumber (BREthereumBlockHeader header) {
    return header->number;
}

extern uint64_t
blockHeaderGetTimestamp (BREthereumBlockHeader header) {
    return header->timestamp;
}

extern uint64_t
blockHeaderGetDifficulty (BREthereumBlockHeader header) {
    return header->difficulty;
}

extern uint64_t
blockHeaderGetGasUsed (BREthereumBlockHeader header) {
    return header->gasUsed;
}

// ...

extern uint64_t
blockHeaderGetNonce (BREthereumBlockHeader header) {
    return header->nonce;
}

extern BREthereumBoolean
blockHeaderMatch (BREthereumBlockHeader header,
                  BREthereumBloomFilter filter) {
    return bloomFilterMatch(header->logsBloom, filter);
}

extern BREthereumBoolean
blockHeaderMatchAddress (BREthereumBlockHeader header,
                         BREthereumAddress address) {
    return (ETHEREUM_BOOLEAN_IS_TRUE(blockHeaderMatch(header, bloomFilterCreateAddress(address)))
            || ETHEREUM_BOOLEAN_IS_TRUE(blockHeaderMatch(header, logTopicGetBloomFilterAddress(address)))
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern size_t
blockHeaderHashValue (const void *h)
{
    return hashSetValue(&((BREthereumBlockHeader) h)->hash);
}

extern int
blockHeaderHashEqual (const void *h1, const void *h2) {
    return hashSetEqual(&((BREthereumBlockHeader) h1)->hash,
                        &((BREthereumBlockHeader) h2)->hash);
}

//
// Block Header RLP Encode / Decode
//
//
static BRRlpItem
blockHeaderRlpEncodeItem (BREthereumBlockHeader header,
                          BREthereumBoolean withNonce,
                          BRRlpCoder coder) {
    BRRlpItem items[15];
    size_t itemsCount = ETHEREUM_BOOLEAN_IS_TRUE(withNonce) ? 15 : 13;

    items[ 0] = hashRlpEncode(header->parentHash, coder);
    items[ 1] = hashRlpEncode(header->ommersHash, coder);
    items[ 2] = addressRawRlpEncode(header->beneficiary, coder);
    items[ 3] = hashRlpEncode(header->stateRoot, coder);
    items[ 4] = hashRlpEncode(header->transactionsRoot, coder);
    items[ 5] = hashRlpEncode(header->receiptsRoot, coder);
    items[ 6] = bloomFilterRlpEncode(header->logsBloom, coder);
    items[ 7] = rlpEncodeItemUInt64(coder, header->difficulty, 0);
    items[ 8] = rlpEncodeItemUInt64(coder, header->number, 0);
    items[ 9] = rlpEncodeItemUInt64(coder, header->gasLimit, 0);
    items[10] = rlpEncodeItemUInt64(coder, header->gasUsed, 0);
    items[11] = rlpEncodeItemUInt64(coder, header->timestamp, 0);
    items[12] = rlpEncodeItemBytes(coder, header->extraData, header->extraDataCount);

    if (ETHEREUM_BOOLEAN_IS_TRUE(withNonce)) {
        items[13] = hashRlpEncode(header->mixHash, coder);
        items[14] = rlpEncodeItemUInt64(coder, header->nonce, 0);
    }

    return rlpEncodeListItems(coder, items, itemsCount);
}

extern BRRlpData
blockHeaderEncodeRLP (BREthereumBlockHeader header, BREthereumBoolean withNonce) {
    BRRlpData result;

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem encoding = blockHeaderRlpEncodeItem(header, withNonce, coder);

    rlpDataExtract(coder, encoding, &result.bytes, &result.bytesCount);
    rlpCoderRelease(coder);

    return result;
}

static BREthereumBlockHeader
blockHeaderRlpDecodeItem (BRRlpItem item, BRRlpCoder coder) {
    BREthereumBlockHeader header = (BREthereumBlockHeader) calloc (1, sizeof(struct BREthereumBlockHeaderRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (13 == itemsCount || 15 == itemsCount);

    header->hash = hashCreateEmpty();

    header->parentHash = hashRlpDecode(items[0], coder);
    header->ommersHash = hashRlpDecode(items[1], coder);
    header->beneficiary = addressRawRlpDecode(items[2], coder);
    header->stateRoot = hashRlpDecode(items[3], coder);
    header->transactionsRoot = hashRlpDecode(items[4], coder);
    header->receiptsRoot = hashRlpDecode(items[5], coder);
    header->logsBloom = bloomFilterRlpDecode(items[6], coder);
    header->difficulty = rlpDecodeItemUInt64(coder, items[7], 0);
    header->number = rlpDecodeItemUInt64(coder, items[8], 0);
    header->gasLimit = rlpDecodeItemUInt64(coder, items[9], 0);
    header->gasUsed = rlpDecodeItemUInt64(coder, items[10], 0);
    header->timestamp = rlpDecodeItemUInt64(coder, items[11], 0);

    BRRlpData extraData = rlpDecodeItemBytes(coder, items[12]);
    memset (header->extraData, 0, 32);
    memcpy (header->extraData, extraData.bytes, extraData.bytesCount);
    header->extraDataCount = extraData.bytesCount;

    if (15 == itemsCount) {
        header->mixHash = hashRlpDecode(items[13], coder);
        header->nonce = rlpDecodeItemUInt64(coder, items[14], 0);
    }

    return header;

}

extern BREthereumBlockHeader
blockHeaderDecodeRLP (BRRlpData data) {
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = rlpGetItem (coder, data);

    BREthereumBlockHeader header = blockHeaderRlpDecodeItem(item, coder);

    // The BlockHeader/Block hash is the KECCAK hash of the BlockerHeader's RLP encoding for
    // a BlockHeader with a nonce...
    if (0 != header->nonce)
        header->hash = hashCreateFromData (data);

    rlpCoderRelease(coder);

    return header;
}

//
// An Ethereum Block
//
// The block in Ethereum is the collection of relevant pieces of information (known as the
// block header), H, together with information corresponding to the comprised transactions, T,
// and a set of other block headers U that are known to have a parent equal to the present block’s
// parent’s parent (such blocks are known as ommers).
//
struct BREthereumBlockRecord {
    BREthereumBlockHeader header;
    BREthereumBlockHeader *ommers;
    BREthereumTransaction *transactions;
};

extern BREthereumBlock
createBlockMinimal(BREthereumHash hash, uint64_t number, uint64_t timestamp) {
    BREthereumBlock block = (BREthereumBlock) calloc (1, sizeof (struct BREthereumBlockRecord));
    block->header = createBlockHeaderMinimal(hash, number, timestamp);
    array_new(block->ommers, 0);
    array_new(block->transactions, 0);
    return block;
}

extern BREthereumBlock
createBlock (BREthereumBlockHeader header,
             BREthereumBlockHeader ommers[], size_t ommersCount,
             BREthereumTransaction transactions[], size_t transactionCount) {
    BREthereumBlock block = (BREthereumBlock) calloc (1, sizeof (struct BREthereumBlockRecord));

    array_new (block->ommers, ommersCount);
    for (int i = 0; i < ommersCount; i++)
        array_add (block->ommers, ommers[i]);

    array_new(block->transactions, transactionCount);
    for (int i = 0; i < transactionCount; i++)
        array_add (block->transactions, transactions[i]);

    return block;
}

extern BREthereumBoolean
blockIsValid (BREthereumBlock block,
              BREthereumBoolean skipHeaderValidation) {
    if (ETHEREUM_BOOLEAN_IS_FALSE(skipHeaderValidation)
        && ETHEREUM_BOOLEAN_IS_FALSE(blockHeaderIsValid(blockGetHeader(block))))
        return ETHEREUM_BOOLEAN_FALSE;

    // TODO: Validate transactions - Merkle Root
    return ETHEREUM_BOOLEAN_TRUE;
}

extern BREthereumBlockHeader
blockGetHeader (BREthereumBlock block) {
    return block->header;
}

extern unsigned long
blockGetTransactionsCount (BREthereumBlock block) {
    return array_count(block->transactions);
}

extern BREthereumTransaction
blockGetTransaction (BREthereumBlock block, unsigned int index) {
    return (index < array_count(block->transactions)
            ? block->transactions[index]
            : NULL);
}

extern unsigned long
blockGetOmmersCount (BREthereumBlock block) {
    return array_count(block->ommers);
}

extern BREthereumBlockHeader
blockGetOmmer (BREthereumBlock block, unsigned int index) {
    return (index < array_count(block->ommers)
            ? block->ommers[index]
            : NULL);
}

extern BREthereumHash
blockGetHash (BREthereumBlock block) {
    return  block->header->hash;
}

extern uint64_t
blockGetNumber (BREthereumBlock block) {
    return block->header->number;
}

extern uint64_t
blockGetTimestamp (BREthereumBlock block) {
    return block->header->timestamp;
}

private_extern void
blockFree (BREthereumBlock block) {
    blockHeaderFree(block->header);
    free (block);
}

//
// Block RLP Encode / Decode
//
static BRRlpItem
blockTransactionsRlpEncodeItem (BREthereumBlock block,
                                BREthereumNetwork network,
                                BRRlpCoder coder) {
    size_t itemsCount = array_count(block->transactions);
    BRRlpItem items[itemsCount];

    for (int i = 0; i < itemsCount; i++)
        items[i] = transactionRlpEncodeItem(block->transactions[i],
                                            network,
                                            TRANSACTION_RLP_SIGNED,
                                            coder);

    return rlpEncodeListItems(coder, items, itemsCount);
}

static BREthereumTransaction *
blockTransactionsRlpDecodeItem (BRRlpItem item,
                                BREthereumNetwork network,
                                BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BREthereumTransaction *transactions;
    array_new(transactions, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumTransaction transaction = transactionRlpDecodeItem(items[i],
                                                                     network,
                                                                     TRANSACTION_RLP_SIGNED,
                                                                     coder);
        array_add(transactions, transaction);
    }

    return transactions;
}

static BRRlpItem
blockOmmersRlpEncodeItem (BREthereumBlock block,
                          BRRlpCoder coder) {
    size_t itemsCount = array_count(block->ommers);
    BRRlpItem items[itemsCount];

    for (int i = 0; i < itemsCount; i++)
        items[i] = blockHeaderRlpEncodeItem (block->ommers[i],
                                             ETHEREUM_BOOLEAN_TRUE,
                                             coder);

    return rlpEncodeListItems(coder, items, itemsCount);
}

extern BREthereumBlockHeader *
blockOmmersRlpDecodeItem (BRRlpItem item,
                                BREthereumNetwork network,
                                BRRlpCoder coder) {
    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);

    BREthereumBlockHeader *headers;
    array_new(headers, itemsCount);

    for (int i = 0; i < itemsCount; i++) {
        BREthereumBlockHeader header = blockHeaderRlpDecodeItem(items[i], coder);
        array_add(headers, header);
    }

    return headers;
}

//
// Block Encode
//
static BRRlpItem
blockRlpEncodeItem (BREthereumBlock block,
                    BREthereumNetwork network,
                    BRRlpCoder coder) {
    BRRlpItem items[3];

    items[0] = blockHeaderRlpEncodeItem(block->header, ETHEREUM_BOOLEAN_TRUE, coder);
    items[1] = blockTransactionsRlpEncodeItem(block, network, coder);
    items[2] = blockOmmersRlpEncodeItem(block, coder);

    return rlpEncodeListItems(coder, items, 3);
}

extern BRRlpData
blockEncodeRLP (BREthereumBlock block, BREthereumNetwork network) {
    BRRlpData result;

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem encoding = blockRlpEncodeItem(block, network, coder);

    rlpDataExtract(coder, encoding, &result.bytes, &result.bytesCount);
    rlpCoderRelease(coder);

    return result;
}

//
// Block Decode
//
static BREthereumBlock
blockRlpDecodeItem (BRRlpItem item, // network
                    BREthereumNetwork network,
                    BRRlpCoder coder) {
    BREthereumBlock block = calloc (1, sizeof(struct BREthereumBlockRecord));
    memset (block, 0, sizeof(struct BREthereumBlockRecord));

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (3 == itemsCount);

    block->header = blockHeaderRlpDecodeItem(items[0], coder);
    block->transactions = blockTransactionsRlpDecodeItem(items[1], network, coder);
    block->ommers = blockOmmersRlpDecodeItem(items[2], network, coder);

    return block;
}

extern BREthereumBlock
blockDecodeRLP (BRRlpData data,
                BREthereumNetwork network) {
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = rlpGetItem (coder, data);

    BREthereumBlock block = blockRlpDecodeItem(item, network, coder);

    rlpCoderRelease(coder);
    return block;
}

//
// Genesis Block
//
// We should extract these blocks from the Ethereum Blockchain so as to have the definitive
// data.  

/*
 Appendix I. Genesis Block and is specified thus:
 The genesis block is 15 items,
 (0-256 , KEC(RLP()), 0-160 , stateRoot, 0, 0, 0-2048 , 2^17 , 0, 0, 3141592, time, 0, 0-256 , KEC(42), (), ()􏰂
 Where 0256 refers to the parent hash, a 256-bit hash which is all zeroes; 0160 refers to the
 beneficiary address, a 160-bit hash which is all zeroes; 02048 refers to the log bloom,
 2048-bit of all zeros; 217 refers to the difficulty; the transaction trie root, receipt
 trie root, gas used, block number and extradata are both 0, being equivalent to the empty
 byte array. The sequences of both ommers and transactions are empty and represented by
 (). KEC(42) refers to the Keccak hash of a byte array of length one whose first and only byte
 is of value 42, used for the nonce. KEC(RLP()) value refers to the hash of the ommer list in
 RLP, both empty lists.

 The proof-of-concept series include a development premine, making the state root hash some
 value stateRoot. Also time will be set to the initial timestamp of the genesis block. The
 latest documentation should be consulted for those values.
 */

// Ideally, we'd statically initialize the Genesis blocks.  But, we don't have static
// initializer for BREthereumHash, BREthereumAddress nor BREthereumBlookFilter.  Therefore,
// will define `initializeGenesisBlocks(void)` to convert hex-strings into the binary values.
//
// The following is expanded for illustration only; it is filled below.
static struct BREthereumBlockHeaderRecord genesisMainnetBlockHeaderRecord = {

    // BREthereumHash hash;
    EMPTY_HASH_INIT,

    // BREthereumHash parentHash;
    EMPTY_HASH_INIT,

    // BREthereumHash ommersHash;
    EMPTY_HASH_INIT,

    // BREthereumAddressRaw beneficiary;
    EMPTY_ADDRESS_INIT,

    // BREthereumHash stateRoot;
    EMPTY_HASH_INIT,

    // BREthereumHash transactionsRoot;
    EMPTY_HASH_INIT,

    // BREthereumHash receiptsRoot;
    EMPTY_HASH_INIT,

    // BREthereumBloomFilter logsBloom;
    EMPTY_BLOOM_FILTER_INIT,

    // uint64_t difficulty;
    1 << 17,

    // uint64_t number;
    0,

    // uint64_t gasLimit;
    0x1388,

    // uint64_t gasUsed;
    0,

    // uint64_t timestamp;
    0,

    // uint8_t extraData [32];
    { 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
      0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0 },

    // uint8_t extraDataCount;
    0,

#if BLOCK_HEADER_NEEDS_SEED_HASH == 1
//    BREthereumHash seedHash;
    EMPTY_HASH_INIT,
#endif

    // BREthereumHash mixHash;
    EMPTY_HASH_INIT,

    // uint64_t nonce;
    0x0000000000000042
};
static struct BREthereumBlockHeaderRecord genesisTestnetBlockHeaderRecord;
static struct BREthereumBlockHeaderRecord genesisRinkebyBlockHeaderRecord;

const BREthereumBlockHeader ethereumMainnetBlockHeader = &genesisMainnetBlockHeaderRecord;
const BREthereumBlockHeader ethereumTestnetBlockHeader = &genesisTestnetBlockHeaderRecord;
const BREthereumBlockHeader ethereumRinkebyBlockHeader = &genesisRinkebyBlockHeaderRecord;

static void
initializeGenesisBlocks (void);

extern const BREthereumBlockHeader
networkGetGenesisBlockHeader (BREthereumNetwork network) {
    static int needInitializeGenesisBlocks = 1;

    if (needInitializeGenesisBlocks) {
        needInitializeGenesisBlocks = 0;
        initializeGenesisBlocks();
    }

    if (network == ethereumMainnet) return ethereumMainnetBlockHeader;
    if (network == ethereumTestnet) return ethereumTestnetBlockHeader;
    if (network == ethereumRinkeby) return ethereumRinkebyBlockHeader;
    assert (0);
}

static void
initializeGenesisBlocks (void) {
    BREthereumBlockHeader header;

    // Mainnet
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {
            "hash":"0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "miner":"0x0000000000000000000000000000000000000000",
            "stateRoot":"0xd7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544",
            "transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",

            "difficulty":"0x400000000",
            "number":"0x0",
            "gasLimit":"0x1388",
            "gasUsed":"0x0",
            "timestamp":"0x0",

            "extraData":"0x11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000042",

            "size":"0x21c",
            "totalDifficulty":"0x400000000",

            "transactions":[],
            "uncles":[]
        }}
     */
    header = &genesisMainnetBlockHeaderRecord;
    header->hash = hashCreate("0xd4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3");
    header->parentHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = hashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = addressRawCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = hashCreate("0xd7f8974fb5ac78d9ac099b9ad5018bedc2ce0a72dad1827a1709da30580f0544");
    header->transactionsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = bloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = 0x400000000;
    header->number = 0x0;
    header->gasLimit = 0x1388;
    header->gasUsed = 0x0;
    header->timestamp = 0x0;
    decodeHex(header->extraData, 32, "11bbe8db4e347b4e8c937c1c8370e4b5ed33adb3db69cbdb7a38e1e50b1b82fa", 64);
    header->extraDataCount = 32;
    header->mixHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000042;

    // Testnet
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {"difficulty":"0x100000",
            "extraData":"0x3535353535353535353535353535353535353535353535353535353535353535",
            "gasLimit":"0x1000000",
            "gasUsed":"0x0",
            "hash":"0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "miner":"0x0000000000000000000000000000000000000000",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000042",
            "number":"0x0",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "size":"0x21c",
            "stateRoot":"0x217b0bbcfb72e2d57e28f33cb361b9983513177755dc3f33ce3e7022ed62b77b",
            "timestamp":"0x0",
            "totalDifficulty":"0x100000",
            "transactions":[],
            "transactionsRoot": "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "uncles":[]}}
     */
    header = &genesisTestnetBlockHeaderRecord;
    header->hash = hashCreate("0x41941023680923e0fe4d74a34bdac8141f2540e3ae90623718e47d66d1ca4a2d");
    header->parentHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = hashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = addressRawCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = hashCreate("0x217b0bbcfb72e2d57e28f33cb361b9983513177755dc3f33ce3e7022ed62b77b");
    header->transactionsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = bloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = 0x100000;
    header->number = 0x0;
    header->gasLimit = 0x1000000;
    header->gasUsed = 0x0;
    header->timestamp = 0x0;
    decodeHex(header->extraData, 32, "3535353535353535353535353535353535353535353535353535353535353535", 64);
    header->extraDataCount = 32;
    header->mixHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000042;

    // Rinkeby
    /*
    {"jsonrpc":"2.0","id":1,"result":
        {"difficulty":"0x1",
            "extraData":"0x52657370656374206d7920617574686f7269746168207e452e436172746d616e42eb768f2244c8811c63729a21a3569731535f067ffc57839b00206d1ad20c69a1981b489f772031b279182d99e65703f0076e4812653aab85fca0f00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "gasLimit":"0x47b760",
            "gasUsed":"0x0",
            "hash":"0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177",
            "logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "miner":"0x0000000000000000000000000000000000000000",
            "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "nonce":"0x0000000000000000",
            "number":"0x0",
            "parentHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
            "receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
            "size":"0x29a",
            "stateRoot":"0x53580584816f617295ea26c0e17641e0120cab2f0a8ffb53a866fd53aa8e8c2d",
            "timestamp":"0x58ee40ba",
            "totalDifficulty":"0x1",
            "transactions":[],
            "transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421",
            "uncles":[]}}
     */
    header = &genesisRinkebyBlockHeaderRecord;
    header->hash = hashCreate("0x6341fd3daf94b748c72ced5a5b26028f2474f5f00d824504e4fa37a75767e177");
    header->parentHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->ommersHash = hashCreate("0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    header->beneficiary = addressRawCreate("0x0000000000000000000000000000000000000000");
    header->stateRoot = hashCreate("0x53580584816f617295ea26c0e17641e0120cab2f0a8ffb53a866fd53aa8e8c2d");
    header->transactionsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->receiptsRoot = hashCreate("0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    header->logsBloom = bloomFilterCreateString("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    header->difficulty = 0x1;
    header->number = 0x0;
    header->gasLimit = 0x47b760;
    header->gasUsed = 0x0;
    header->timestamp = 0x58ee40ba;
    // TODO: Rinkeby ExtraData is oversized... ignore.
    decodeHex(header->extraData, 32, "3535353535353535353535353535353535353535353535353535353535353535", 64);
    header->extraDataCount = 32;
    header->mixHash = hashCreate("0x0000000000000000000000000000000000000000000000000000000000000000");
    header->nonce = 0x0000000000000000;
}
