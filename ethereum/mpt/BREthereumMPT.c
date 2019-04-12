//
//  BREthereumMPT.c
//  Core
//
//  Created by Ed Gamble on 8/21/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "support/BRAssert.h"
#include "BREthereumMPT.h"

#undef MPT_SHOW_PROOF_NODES

/// MARK: - MPT Node

typedef struct BREthereumMPTNodeRecord *BREthereumMPTNode;

struct BREthereumMPTNodeRecord {
    BREthereumMPTNodeType type;
    union {
        struct {
            BREthereumData path;  // data w/ each byte a nibble a/ preface stripped!
            BRRlpData value;
        } leaf;

        struct {
            BREthereumData path;  // data w/ each byte a nibble a/ preface stripped!
            BREthereumHash key;
        } extension;

        struct {
            BREthereumHash keys[16];
            BRRlpData value;
        } branch;
    } u;
};

static BREthereumMPTNode
mptNodeCreate (BREthereumMPTNodeType type) {
    BREthereumMPTNode node = calloc (1, sizeof (struct BREthereumMPTNodeRecord));
    node->type = type;
    return node;
}

static void
mptNodeRelease (BREthereumMPTNode node) {
    if (NULL == node) return;  // On RLP coding error during 'nodes' processing
    switch (node->type) {
        case MPT_NODE_LEAF:
            dataRelease(node->u.leaf.path);
            rlpDataRelease(node->u.leaf.value);
            break;

        case MPT_NODE_EXTENSION:
            dataRelease (node->u.extension.path);
            break;

        case MPT_NODE_BRANCH:
            rlpDataRelease (node->u.branch.value);
            break;
    }
    free (node);
}

static BRRlpData
mptNodeGetValue (BREthereumMPTNode node,
                 BREthereumBoolean *found) {
    switch (node->type) {
        case MPT_NODE_LEAF:
            *found = ETHEREUM_BOOLEAN_TRUE;
            return node->u.leaf.value;

        case MPT_NODE_EXTENSION:
            *found = ETHEREUM_BOOLEAN_TRUE;
            return (BRRlpData) { 0, NULL };

        case MPT_NODE_BRANCH:
            *found = ETHEREUM_BOOLEAN_TRUE;
            return node->u.branch.value;
    }
}

static BREthereumData
mptNodeGetPath (BREthereumMPTNode node) {
    switch (node->type) {
        case MPT_NODE_LEAF:      return node->u.leaf.path;
        case MPT_NODE_EXTENSION: return node->u.extension.path;
        case MPT_NODE_BRANCH:    BRFail();
    }
}

static size_t
mptNodeConsume (BREthereumMPTNode node, uint8_t *key) {
    switch (node->type) {
        case MPT_NODE_LEAF:
        case MPT_NODE_EXTENSION: {
            BREthereumData path = mptNodeGetPath(node);
            for (size_t index = 0; index < path.count; index++)
                if (key[index] != path.bytes[index])
                    return 0;
            return path.count;
        }

        case MPT_NODE_BRANCH: {
            // We'll consume one byte if the node's key is not an empty hash
            return (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual (node->u.branch.keys[key[0]],
                                                         EMPTY_HASH_INIT))
                    ? 0
                    : 1);
        }
    }
}

#define NIBBLE_UPPER(x)     (0x0f & ((x) >> 4))
#define NIBBLE_LOWER(x)     (0x0f & ((x) >> 0))

#define NIBBLE_GET(x, upper) (0x0f & ((x) >> ((upper) ? 4 : 0)))

static BREthereumMPTNode
mptNodeDecode (BRRlpItem item,
               BRRlpCoder coder) {
    BREthereumMPTNode node = NULL;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);
    if (17 != itemsCount && 2 != itemsCount) { rlpCoderSetFailed (coder); return NULL; }

    switch (itemsCount) {
        case 2: {
            // Decode, skipping to the bytes (w/o the RLP length prefix)
            BRRlpData pathData = rlpDecodeBytesSharedDontRelease(coder, items[0]);
            assert (0 != pathData.bytesCount);

            // Extract the nodeType nibble; determine `type` and `padded`
            uint8_t nodeTypeNibble = NIBBLE_UPPER(pathData.bytes[0]);

            BREthereumMPTNodeType type = (nodeTypeNibble < 2
                                          ? MPT_NODE_EXTENSION
                                          : MPT_NODE_LEAF);
            // we are 'padded' if the nodeTypeNibble is even
            int padded = 0 == (nodeTypeNibble & 0x01);

            // Fill the encoded path
            size_t   pathCount = 2 * pathData.bytesCount - 1 - padded;
            uint8_t *pathBytes = malloc (pathCount);

            BREthereumData path = { pathCount, pathBytes };

            /*
             > [ 1, 2, 3, 4, 5, ...]
             '11 23 45'
             > [ 0, 1, 2, 3, 4, 5, ...]
             '00 01 23 45'
             > [ 0, f, 1, c, b, 8, 10]
             '20 0f 1c b8'
             > [ f, 1, c, b, 8, 10]
             '3f 1c b8'
             */

            if (!padded) *pathBytes++ = NIBBLE_LOWER(pathData.bytes[0]);

            for (size_t index = 1; index < pathData.bytesCount; index++) {
                uint8_t byte = pathData.bytes[index];
                *pathBytes++ = NIBBLE_UPPER (byte); // padded);
                *pathBytes++ = NIBBLE_LOWER (byte); // !padded);
            }

            node = mptNodeCreate(type);
            switch (type) {
                case MPT_NODE_LEAF:
                    node->u.leaf.path = path;
                    node->u.leaf.value = rlpDecodeBytes (coder, items[1]);
                    break;

                case MPT_NODE_EXTENSION:
                    node->u.extension.path = path;
                    node->u.extension.key = hashRlpDecode (items[1], coder);
                    break;

                case MPT_NODE_BRANCH:
                    assert (0);
            }
            break;
        }

        case 17: {
            node = mptNodeCreate(MPT_NODE_BRANCH);
            for (size_t index = 0; index < 16; index++) {
                BRRlpData data = rlpGetDataSharedDontRelease (coder, items[index]);
                // Either a hash (0x<32 bytes>) or empty (0x)
                node->u.branch.keys[index] = (0 == data.bytesCount || 1 == data.bytesCount
                                              ? EMPTY_HASH_INIT
                                              : hashRlpDecode(items[index], coder));
            }
            node->u.branch.value = rlpGetData (coder, items[16]);
            break;
        }
    }
    return node;
}

/// MARK: - MPT Node Path

struct BREthereumMPTNodePathRecord {
    BRArrayOf(BREthereumMPTNode) nodes;
};

static BREthereumMPTNodePath
mptNodePathCreate (BRArrayOf(BREthereumMPTNode) nodes) {
    BREthereumMPTNodePath path = malloc (sizeof (struct BREthereumMPTNodePathRecord));
    path->nodes = nodes;
    return path;
}

extern void
mptNodePathRelease (BREthereumMPTNodePath path) {
    for (size_t index = 0; index < array_count (path->nodes); index++)
        mptNodeRelease (path->nodes[index]);
    array_free (path->nodes);
    free (path);
}

extern void
mptNodePathsRelease (BRArrayOf(BREthereumMPTNodePath) paths) {
    if (NULL != paths) {
        size_t count = array_count(paths);
        for (size_t index = 0; index < count; index++)
            mptNodePathRelease(paths[index]);
        array_free (paths);
    }
}

extern BREthereumMPTNodePath
mptNodePathDecode (BRRlpItem item,
                   BRRlpCoder coder) {
    size_t itemsCount;
    const BRRlpItem *items = rlpDecodeList (coder, item, &itemsCount);

    BRArrayOf (BREthereumMPTNode) nodes;
    array_new (nodes, itemsCount);
    for (size_t index = 0; index < itemsCount; index++)
        array_add (nodes, mptNodeDecode (items[index], coder));

    return mptNodePathCreate(nodes);
}

extern BREthereumMPTNodePath
mptNodePathDecodeFromBytes (BRRlpItem item,
                            BRRlpCoder coder) {
    size_t itemsCount = 0;
    BRRlpItem *items = (BRRlpItem *) rlpDecodeList(coder, item, &itemsCount);

    BRArrayOf (BREthereumMPTNode) nodes;
    array_new (nodes, itemsCount);
    for (size_t index = 0; index < itemsCount; index++) {
        // items[index] holds bytes as the RLP encoding of MPT nodes.  We'll decode the bytes
        // and then RLP encode the bytes (but this time as RLP items.... got it??).
        BRRlpData data = rlpDecodeBytesSharedDontRelease (coder, items[index]);
        BRRlpItem item = rlpGetItem(coder, data);
        array_add (nodes, mptNodeDecode (item, coder));
#if defined (MPT_SHOW_PROOF_NODES)
        rlpShowItem (coder, item, "MPTN");
#endif
        rlpReleaseItem (coder, item);
    }

    // TODO: If any above item is decoded improperly, then `nodes` will have NULL values.

    return mptNodePathCreate(nodes);
}

extern BREthereumMPTNode
mptNodePathGetNode (BREthereumMPTNodePath path,
                    BREthereumData key) {
    size_t  keyEncodedCount = 2 * key.count;
    uint8_t keyEncoded [keyEncodedCount];

    // Fill the key
    for (size_t index = 0; index < key.count; index++) {
        uint8_t byte = key.bytes[index];
        keyEncoded [2 * index + 0] = NIBBLE_UPPER(byte);
        keyEncoded [2 * index + 1] = NIBBLE_LOWER(byte);
    }

    uint8_t keyEncodedIndex = 0;

    // Walk the nodes, consuming the key if possible.
    for (size_t index = 0; index < array_count (path->nodes); index++) {
        BREthereumMPTNode node = path->nodes[index];
        size_t keyEncodedIncrement = mptNodeConsume (node, &keyEncoded[keyEncodedIndex]);

        // nothing consumed, definitively node missed
        if (0 == keyEncodedIncrement)
            break;

        keyEncodedIndex += keyEncodedIncrement;

        // If all of key is consumed, then done.
        if (keyEncodedCount == keyEncodedIndex) {
            // We have a screwy case here... we've seen a subsequent 'leaf' node, without any
            // path, holding the 'value'.  Not sure why (Parity bug submitted); we'll try to
            // pick out that node
            if (index + 1 + 1 == array_count(path->nodes) &&       // one node remains...
                MPT_NODE_LEAF == path->nodes[index + 1]->type &&   // it is a leaf node
                0 == path->nodes[index + 1]->u.leaf.path.count)    // it has no path
                return path->nodes[index + 1];

            return node;
        }
    }

    return NULL;
}

extern BREthereumBoolean
mptNodePathIsValid (BREthereumMPTNodePath path,
                    BREthereumData key) {
    return AS_ETHEREUM_BOOLEAN (NULL != mptNodePathGetNode (path, key));
}

extern BRRlpData
mptNodePathGetValue (BREthereumMPTNodePath path,
                      BREthereumData key,
                      BREthereumBoolean *found) {
    BREthereumMPTNode node = mptNodePathGetNode (path, key);
    if (NULL == node) {
        *found = ETHEREUM_BOOLEAN_FALSE;
        return (BRRlpData) { 0, NULL };
    }
    else return rlpDataCopy (mptNodeGetValue (node, found));
}

extern BREthereumData
mptNodePathGetKeyFragment (BREthereumMPTNodePath path) {
    BREthereumData result = { 64, malloc(64) };
    size_t index = 0;

    for (size_t n = 0; n < array_count (path->nodes); n++) {
        BREthereumMPTNode node = path->nodes[n];
        switch (node->type) {
            case MPT_NODE_LEAF:
            case MPT_NODE_EXTENSION: {
                BREthereumData key = mptNodeGetPath (node);
                memcpy (&result.bytes[index], key.bytes, key.count);
                index += key.count;
                break;
            }

            case MPT_NODE_BRANCH:
                result.bytes[index] = 0xf;
                index++;
                break;
        }
    }

    return result;
}

extern BREthereumData
mptKeyGetFromUInt64 (uint64_t value) {
    size_t count = sizeof (uint64_t);
    BREthereumData data = { count, malloc (count) };

    // TODO: Assumes Little Endian
    uint8_t *valueBytes = (uint8_t *) &value;

    // TODO: Not swapping nibbles??
    for (size_t index = 0; index < count; index++)
        data.bytes[index] = valueBytes[count - 1 - index];

    return data;
}

extern BREthereumData
mptKeyGetFromHash (BREthereumHash hash) {
    size_t count = sizeof (hash.bytes);
    BREthereumData data = { count, malloc(count) };

    memcpy (data.bytes, hash.bytes, count);

    return data;
}

/*
 https://github.com/ethereum/wiki/wiki/Patricia-Tree

 When traversing paths in nibbles, we may end up with an odd number of nibbles to traverse, but
 because all data is stored in bytes format, it is not possible to differentiate between, for
 instance, the nibble 1, and the nibbles 01 (both must be stored as <01>). To specify odd length,
 the partial path is prefixed with a flag.

 The flagging of both odd vs. even remaining partial path length and leaf vs. extension node as
 described above reside in the first nibble of the partial path of any 2-item node. They result
 in the following:

 hex char    bits    |    node type partial     path length
 ----------------------------------------------------------
 0        0000    |       extension              even
 1        0001    |       extension              odd
 2        0010    |   terminating (leaf)         even
 3        0011    |   terminating (leaf)         odd

 For even remaining path length (0 or 2), another 0 "padding" nibble will always follow.
 */


/*
 ETH: LES: Send: [ LES,     GetProofsV2 ] =>  35.193.192.189
 ETH: SEND: L  2: [
 ETH: SEND:   I  1: 0x1f
 ETH: SEND:   L  2: [
 ETH: SEND:     I  0: 0x
 ETH: SEND:     L  1: [
 ETH: SEND:       L  4: [
 ETH: SEND:         I 32: 0x93965e97196014f11f29352c501bd583ea9619cd0b965643f52c8c5bc179d2a7
 ETH: SEND:         I  0: 0x
 ETH: SEND:         I 32: 0x1372c6cc9ad4698bc90e4f827e36bf3930b9a02aa2d626d71dddbf3b9e9eb9d4
 ETH: SEND:         I  0: 0x
 ETH: SEND:       ]
 ETH: SEND:     ]
 ETH: SEND:   ]
 ETH: SEND: ]
 ETH: RECV: L  3: [
 ETH: RECV:   I  0: 0x
 ETH: RECV:   I  4: 0x11aaf191
 ETH: RECV:   L  8: [
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0xdd6eff5bf9d98c967b5197f4a258bc4d3b469ea88af6ba130c1ecd491ea823b5
 ETH: RECV:**1    I 32: 0x280d5fd8f2381aed8dedeb1954c37677109b940a7feb546c809baf7b2a052028
 ETH: RECV:       I 32: 0xff123f796cbaccd33bdba5c1d48de48cc5e1bbef5d38cbb890664b71f86c8247
 ETH: RECV:       I 32: 0xfc287d9f867579881a77a157f131c2d1b0c48a28b974c031bba2f1ca4cc01d38
 ETH: RECV:       I 32: 0x395126bce7d8a7fdd1c866c0036274468ad09026c682899230f1186a909efb31
 ETH: RECV:       I 32: 0x47ddae583d54b14722bc8c63ccb2443e66d395ede99f3429e14ef7bc547d67b8
 ETH: RECV:       I 32: 0xf334802bafcb8e4da1ac141b573897ca58b4c72a6487b19758aa5ff692baba20
 ETH: RECV:       I 32: 0x92a062685557210b35ac0a4de0aab3193c7c6d212bac0f2fddacb8192e51aabc
 ETH: RECV:       I 32: 0xf450bae353e730b673a0b41cb6f6773df7aedde2906bead65880cb71ff6fa18f
 ETH: RECV:       I 32: 0xd71396b65e93f142991cc0297d34c8723a66a2a6b82346e2e836b0f57f068581
 ETH: RECV:       I 32: 0xfed94dba13754a742e02b6ac67683e4cb171e4a5ebd69d7ebbd0e977c07d6fe5
 ETH: RECV:       I 32: 0x1450605214f4e90cb3483df9322bc8e549a5934c5990ee046d0085324731fd03
 ETH: RECV:       I 32: 0x46ab678a43a561a35dd8af40d2d58d5e62935f09b405a0436e5778ff68a113c5
 ETH: RECV:       I 32: 0x65ff2172374bb1d712d1cf617d23109dc62a163dc5b64aa954b07e4dfc77497d
 ETH: RECV:       I 32: 0x09b21bc2e432a98747c7427b50511a667dcd2789597daad4eb5ae4de4bbd3571
 ETH: RECV:       I 32: 0x36e2a8a40173ff12b0cbecb915ab3a86d4adbc55e5e9f3a9107be86bc13513e3
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0x290ce85ae46fca97bcfdec7cd41e95aeffe5f408146202db8d3dc429824164c8
 ETH: RECV:       I 32: 0x63bfa3d4f9305fffc7544f29a88890f0bf012ec5dd1597a7cbd70c842a48fab4
 ETH: RECV:       I 32: 0x9a1fbd7fceb14c9705d0ea26f5981a90a9589536775c669a32bace9f1ee48285
 ETH: RECV:**3    I 32: 0x16305c47a2a7414a908ee25bfe112169b03120699f8ff5111fbff02a72501726
 ETH: RECV:       I 32: 0x1058cf2f0fdf61fc7a4ab9f0f706f58c0e135beb28c2ac1a1cdd8f7f9c1accbc
 ETH: RECV:       I 32: 0x4debabcf8077ddbd4b1c8a206a086ee8d247fb58bb38c258b790ec210557e7dd
 ETH: RECV:       I 32: 0x7588ae630049af5a828993f57a1343f94704fb9340503a0d3cdeb58d8e173906
 ETH: RECV:       I 32: 0xa3d965771b674f4ccd038979b70ad1e48c1ef456134c32e247ab10f581c0c3d4
 ETH: RECV:       I 32: 0xdd123c7c5d9d1858c16bbb77e152a03ddd66223777d77a31927e2dcec0bb241d
 ETH: RECV:       I 32: 0x4be8ec0544e342c1d0d4e03c5429eb2b0d0766868c96d2f320f494c0f15db0a9
 ETH: RECV:       I 32: 0xd887f01ccadefe38583fe6b3030a1761871ae74459d3aebc6c85e01a0d407516
 ETH: RECV:       I 32: 0x9e5ba2892879d982fd270c8e033b7f34b70ef084bb9e36f50663b5c41f1ffae0
 ETH: RECV:       I 32: 0xdbe63816fb0f7f8afd1e2c1f6223af3b8f484610bafa71cb95a1d8afeaff13aa
 ETH: RECV:       I 32: 0xf7d9b01b899f371a4a48993534dd1d4930398ef2545a7ed29beaed196def106f
 ETH: RECV:       I 32: 0x502b8ef8659d0a9aef5113ffc9cc94cba8593ceaa3be7ce871bdc301082158ee
 ETH: RECV:       I 32: 0xe07f7605684f98daeedc3786eaeaaf429b6f725a7f0ce06bde257cd78a7ceef3
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0xde69cf9c15cf55722a811849350960c5563d7fd156328e39fad3106fa489c660
 ETH: RECV:       I 32: 0x0967f38ee819d15e17048c555832a8e2e982a9bc1ede890c7f44b409b1efc790
 ETH: RECV:       I 32: 0xfc8a8c8727585c092c13043be610c01924685bca6adc0696cec95c58d8bc4ac2
 ETH: RECV:       I 32: 0xc7e9c79e3428243ee4d0f06fe8b67ed779144a34e2e017b90b05f8ccf94ee467
 ETH: RECV:       I 32: 0x0bef20ea19c1d3b6224bbf7e2a1d8ce3086563809eb307608d1360266188b7e9
 ETH: RECV:       I 32: 0x1ffb84eac0e3cacd30141a4f3b4ed56c33b75b78cc9729659ec144a81b28054f
 ETH: RECV:       I 32: 0xa4e2e72db5b43c5082f21ffd5b6f3663462240d922b3dabd1196c805c92c4bce
 ETH: RECV:**7    I 32: 0x5fca8fc71ec38dc819a5f4ec7883d5b7a1a7eab94e2344402e369750ae86834d
 ETH: RECV:       I 32: 0x45f80cd4d65959136419b7b47466534885ca26cb08121829f90fd91e498aaf42
 ETH: RECV:       I 32: 0xcd0ad7a624c753367a255340ba36f13ba3f9baa07c88b5a06fe8929927e72606
 ETH: RECV:       I 32: 0xf42281c3894dc21260668d6a9e065ade947787c6edd2263aa7d7d8ebc3838fbe
 ETH: RECV:       I 32: 0xaead52abfc99345c6bb335f1498c9396fcb45faa1074e1d1df1a244c7ef2f2da
 ETH: RECV:       I 32: 0xac5998b87d06eee5bfd167ef20229cc1582bd803c3a959d69fad3235fbe62a3a
 ETH: RECV:       I 32: 0xd5ed6649af10e9debac991e96db67350ae6ff1cb27ae49511e86b429b8fd6153
 ETH: RECV:       I 32: 0x6ace648aea9d2077eae34a9565b4f6d368427edaa89bc0baad98ccf6fa5a502f
 ETH: RECV:       I 32: 0x46b71eb22d8442f7e3094fdb88a0b24f2ef812a1bd5c1640ccc88cfa8de68444
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0x06753f029b4b21bc1f7e8c6bad6c926fe87f296988e2cf3a1233afb43786348c
 ETH: RECV:       I 32: 0xa5d0cf84d42025a93c505cf4a9632415892d6baee20a9477701fd9d679e579b3
 ETH: RECV:**2    I 32: 0x25397d3307bde96e8ad6bd8ddbacd8c1102bff0e07aaf9ac602c94846d9f5682
 ETH: RECV:       I 32: 0x5356dd9f157c2920df3eae2b5ae955da537c5018c2dc3d6e2bc459e35591ec92
 ETH: RECV:       I 32: 0x92e0aeaefb95e79fd41829ec95e5c628d68dc862ab269728289f8dfe8b70dbe8
 ETH: RECV:       I 32: 0x858f488e2b2461bdb839c32ecfd89ea1f878528696acf8c155b73dc6e64e0fa7
 ETH: RECV:       I 32: 0xca2e644bec8edbe10f76136e6c84056ed082f1da3c69147427b22e880e161352
 ETH: RECV:       I 32: 0x0b8bfeb600800e18b5e1d4570ca18dc3959d8172441777d62302357a2d44ede9
 ETH: RECV:       I 32: 0xa41012a113990d9cbef6f07f2bfd792c8bc4a9fbc05337bfb1ac63f3ffff1087
 ETH: RECV:       I 32: 0x7d233b18801335ecb39a560fb967e637c1e999b8d742465a11d12f97f735fbdb
 ETH: RECV:       I 32: 0xc6f10427f96a6903eaf665406b2a719b4520d03be784d4cbbd9bfe2f6562cad3
 ETH: RECV:       I 32: 0x448bd863a4cda22d2cbed808ab020f5447dd45a583bf77375d5b6b7afad54097
 ETH: RECV:       I 32: 0xdef270d10d8617f5e899b1b9f1a62dafd0ad48868bacc8ceb7ced07c17c54697
 ETH: RECV:       I 32: 0x17e1768d2bc884118f3ec7d97a298ca2cffcc6e8a7bc4e8293d20874074db200
 ETH: RECV:       I 32: 0x58e5abe652f47523ee3bc18ce58f40464a203e6cc96dd863506a27d47467de46
 ETH: RECV:       I 32: 0x9a5600b669210456cf1e2358a1fe9a473c33dbd816b4fbaf5828fca3eef835ff
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0x9cdf2203e7eda12b627627f4687cbc002f312d8c1bd17a281ef8d3678f281a32
 ETH: RECV:       I 32: 0x3e51c209678c3f2a0b7b64a412158cbff7391692c8cc014c8121a5f6afd2443b
 ETH: RECV:       I 32: 0x6c2ce2b52bf85e495024f48e59881500699ad5a58445f96376135d156e1ba7f0
 ETH: RECV:       I 32: 0x95dc080a615fca9085adb6f2d16ef92ffb4c27c442af84a1aed8eaa8ab70f9b4
 ETH: RECV:       I 32: 0x148b62f0f41ce5228b4b11f041a2a9b6693e1cdba1c246f39ae1a03eabbc93ee
 ETH: RECV:       I 32: 0x298d2364f62757cf0d4f8ae9686c58c2482cf9e8aa34f1f7a9f9b59e87a86145
 ETH: RECV:       I 32: 0x8caea577e7cc28122d9b2c526340f3914ac923f29d24ad5c0428a2394b1c1140
 ETH: RECV:       I 32: 0x4854cbb18c08eb5251cbb1c17826c3277abef94a5de9fc5687b5229da0326d6a
 ETH: RECV:       I 32: 0x99001c2a7c32b145973904355a84cc71b40cb2f887de8041813b89a88a2b990b
 ETH: RECV:       I 32: 0xb251e6b1b65c7538ee7615e30b5964ae60bfebee7cbf8589eac02697b9dddacf
 ETH: RECV:       I 32: 0x9183cbf190f0e4d70d0148545dc25a0b1df34663f4e063d0df02056780203dfc
 ETH: RECV:       I 32: 0x5be7c9e6dbac94404d87ea9a676f37d39cf97e9944114880d3a13a14f52289e7
 ETH: RECV:**c    I 32: 0x8ab1efb6d773f47dcda4982986eca88956caa4e8a81fd4f027135bb6228ba42b
 ETH: RECV:       I 32: 0x491ed0a89e230e8ef3c0ddab98163214759cfc4b737d972a3014658ba632ab6c
 ETH: RECV:       I 32: 0xbe9dffb80a362ad2403f9cbee548d5390b1c0abfbe61b09d688386230aaaf10c
 ETH: RECV:       I 32: 0x257e2eb58f23d241d9969109b1632f7993b97648599ab60e73e2066950935300
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I 32: 0xe7edd9aff5b2109a69f8ed1cdd523e5d5d1b59df110a0fccc7af1566c5c9fa8b
 ETH: RECV:       I 32: 0x164597143946910b4bcc17f19d81dab88a90ccfb9254c3d6277f2fcc9903aa2e
 ETH: RECV:       I 32: 0xf9f07aaeec5e1a176a9a71cc71f6219f3b9d319a71bae258d41249d2b453cb52
 ETH: RECV:       I 32: 0x042b6e9daa7719f9491ce1726668a94db6b5238e0a4cc2d268ab0c1e4c8e00dd
 ETH: RECV:       I 32: 0xbd953830cc0ff4b93b57fbab4b70c650d109791df641e21632caa1bcc6d398c0
 ETH: RECV:       I 32: 0x43a0e24544ad42fd72903c1d57a1bc1e6cd5f37aac3ca092f17d458d6c54c686
 ETH: RECV:**6    I 32: 0xd7bc384edef1bf427c3dad9365457ed2c644fc7dfac225e46dfe084ec77d7897
 ETH: RECV:       I 32: 0x282c0c06cb3dc2030d921761c049d5ddec7044feb462f80d6690cafa82e7afb1
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I 32: 0x12bf9ae36e08f3f358aeb54c05cfdf725e69aad6f19304b6abc69ba8da62d8a1
 ETH: RECV:       I 32: 0x93db5a4c1301033e646af15b6578a964613ac4627c6e847c32fe1879384f450a
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I 32: 0xe49b00e39761840d8bb21d193c6ecc3a755baabf29a89ae49832bbecfd800c54
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I 32: 0xda8497fb6744831483f47e1ba328e7658b7e5934945d58cb38e143f8e76e6fcc
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L 17: [
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I 32: 0xfcfec2196926949d5cef47ae745cb1135eaec85548082294f8aa22f198913aff
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I 32: 0xa4913664739f14eb83bddf15b83a62bca266a408acea51576f78dc651cbe77d9
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:**c    I 32: 0x29454f481919e9d1695a970375fde90beefc08d6b9a610feaed78129b1d48e60
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:       I  0: 0x
 ETH: RECV:     ]
 ETH: RECV:     L  2: [
 ETH: RECV:       I 29: 0x3, c9ad4698bc90e4f827e36bf3930b9a02aa2d626d71dddbf3b9e9eb9d4
 ETH: RECV:       I 83: 0xf851839272128a01beb53ab3f8d9767867a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a0c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
 ETH: RECV:     ]
 ETH: RECV:   ]
 ETH: RECV: ]
 ETH: LES: Recv: [ LES,        ProofsV2 ] <=  35.193.192.189


 0x1372c6c==c9ad4698bc90e4f827e36bf3930b9a02aa2d626d71dddbf3b9e9eb9d4


 0xf851839272128a01beb53ab3f8d9767867a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a0c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
 ETH: RLP:: L  4: [
 ETH: RLP::   I  3: 0x927212
 ETH: RLP::   I 10: 0x01beb53ab3f8d9767867
 ETH: RLP::   I 32: 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
 ETH: RLP::   I 32: 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
 ETH: RLP:: ]

 // GETH Header Proof

 ETH: RECV: L  3: [
 ETH: RECV:   I  0: 0x
 ETH: RECV:   I  4: 0x11e1a300
 ETH: RECV:   L  1: [
 ETH: RECV:     L  2: [
 ETH: RECV:       L 15: [
 ETH: RECV:         <block header>
  ETH: RECV:       ]
 ETH: RECV:       L  1: [
 ETH: RECV:         L  2: [
 ETH: RECV:           I  7: 0x10000000000000
 ETH: RECV:           I 32: 0x2c9e6cb3b3c7f7b2d4550bfae250bcadd40addedece6859f2a6ef2936afdec1f
 ETH: RECV:         ]
 ETH: RECV:       ]
 ETH: RECV:     ]
 ETH: RECV:   ]
 ETH: RECV: ]
 ETH: LES: Recv: [ LES,    HeaderProofs ] <=   35.184.255.33

 */
/*
 // A Parity 'Acount Response' - first of 5 list elements is the 'proof.
 ETH: RECV: L  3: [
 ETH: RECV:   I  0: 0x
 ETH: RECV:   I  5: 0x06fc1f1820
 ETH: RECV:   L  1: [
 ETH: RECV:     L  2: [
 ETH: RECV:       I  1: 0x05
 ETH: RECV:       L  5: [
 ETH: RECV:         L  4: [
 ETH: RECV:           I532: 0xf90211a03a712d5560b6d66ac7e3d5c2f353f98f3d1e8eb57d8808a4468738d8a8998a2ea08cd597e7ec5295f00cc07d8d35e5379eb1101d379d493992724dc931ca1c7eaaa0beec28d1a915ed45c7c018be13e031a08685cd98b3f456ce8c4e1638e545dc97a00272de229cfed792911729093c1d157bc0020e6e7d4d40976ebff9ac1607240aa0939d7c0751da736d51a5ec2cd302527b0c4587672ef593a534ee7a9a3f1a9626a023a9385ffe8f1a51e70e35ca7998170bf185378e5cc1d0a966417cde38be23c4a0793a58f36fcbe76bf9fbb2bbcb19ea51acd360a4df30fec005d13a8bf86c22f4a08c17d3694692ef726b43c53dd8fe8aee41e84c4b43d2d3ef34bfa11c7bc2a7caa05f9a676a864a596847a476bbcfd061899971f44dcfa9250859f48ad8eea7a8cba0381cdfe640dc160a3950beecee2951f2b9eb116dbed4067aeac286c78fb07b87a0c8d5ab1e05fb8d666fc93a597c03304f01f9be06503cb5aadafa247fb9a40635a0be46d054ec47780978674d01aa667ab4a3beff8e543ded5ba2873f3c06eab603a00f5556798e9b88cab09262f6de1a93b4a93d93b74b03803454ebc455409468f4a0b8027d751aea9f22674e1c6229b42e0c35839376e4d5a9b409ecc1f85aaa222ca08e0af74fe8e5a23b87f4f0e79ee2b12595eb141c787c3fdb78f632d3cfea80bea010398823ba2cef79533fde4483...
 ETH: RECV:           I532: 0xf90211a0ed14737976d472d5a0dc31cc084ae669760fc9b3a63ef27b8e073b17d02fa531a07674ac23fe29a9cdc8510918b949a068f67e05bed349fc55a9ee7bc107a1b27da0b456f0f2102a3afc355a39495d7415828f2697299b161cf9d8caf8c75eeda8ffa0e194204f08d529fc75c8c5065888b6b1360e149fc677c162fac05a859960dcd6a045d755db41c21e6f0f73bee654cd687d9ae84ee905c6d46c8dc139ff22959028a0cd0d006e6357eaf3684997ecf8f7b92fe44915b6a2b294e34106a40187e6b90ca074117ce39bb34adb9e06c7b859d0b75cb74526981ab3b6ce975016cdb18c64b4a0bb8c3dded0e6588d7faf43cb71934734cb29ccb9089abeb95849164c0dbd50afa0844f9f0f888f5d98dac8ee20a6ccc5a6d287d066da07bc183b144813be3c67e4a0aeaa105c4b1f3536963f44bedff164c932c970a87606cd44dd1995f3d5f32f43a0ff7f2792a2dd3a0440cb92aebe3ff9171b6822d1bf3905deee7c448cbd178df5a0bbb4a30dd84dd6d301498a0a353df0062a232670e2da5e1cdd48ec23fc5fd23ca0fc1358c994f2e1b55a158021c24494d4c91c5e0e4d5eae2995e058b13adcbb05a063c0ae53e9acf888e0f481d8142539d7b35edaacf633f3b1bc7cb813d476c821a008952b92e181b05afaf5515fc885dcd20cd9fc6eee0e558f504f76e08feb149ba02e9c3743f7e68a9f45b4967e29...
 ETH: RECV:           I500: 0xf901f1a00227242fc68ba11508f0281955260f6e83047813e4b396f15859785cd140cb38a01530cd144b1279d6793ec93ce7398bccc0ea495e93de4fe9eb360d790892eff2a0b0b2eb186eab9c6c5e85849546eb03c5c14344311b56ffb4f2eec6ba24d86011a005907bf762d3dccfe4b837ea9bf8986a40d8437460f37ab1797fd296ce11af0aa03bd826be6fa0b8b94f70a416d983bb13f89bb1e7c8beeec68fb606a83731ea8ba0c854f4bca8a1e166c86c48743c38c3086475256495ed5a45d510a19d38f6e24ca0b75b916508ae302040b03c13aacdaa08a3da4bbb6de37a8903249d168e11a427a0afd9f37c2f6de7ede16e719c0abf25b538ce864a23107f4fc6bbce57ff4c7726a00723d0a49f99a02b7df3ef00481d68e3039e8d91a6ec35469ca55353f14f29d3a0515fa8681d0bb5be4be790639e9c1b7b4b23a663c1da4cc28d23389a296c9c4780a0f49d7be086e62548d0efd728f8c0f992bc411efedfafb970ca81d743e6554b19a079011dfc7a0c0d395f2b1189ddd0b37c881a1795038f40ae717fb213630f750aa0b9a06777e72e8ec747ebf5f7ddd44ce8faa425b16c7578eaff665072c0ff0adea002f29b2f9e2cd183b5c64cfeab74baa38595a491276b020258cf65738abc764da05af330b2d8e30e35080ecc8a369255fb178f3d5494c44d98ea76f771d36990a780
 ETH: RECV:           I147: 0xf891a0646c4869356fa863975773e426530e944debfe815af65e5b4b486f7199b4a001808080a0fde83b87df3f275af35f64a8999ddc908132f9cccf7743365e3ed8feb205b90aa02a1e54363f1736a73ea82fc0df7150bd9f03d536cd89f6603f18b699e751c709808080808080a0a67fc8aba92c88b7ea317c4c2ebf23cd34cdc3f0701f8166376a75b48ef951fc80808080
 ETH: RECV:         ]
 ETH: RECV:         I  0: 0x
 ETH: RECV:         I  0: 0x
 ETH: RECV:         I 32: 0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
 ETH: RECV:         I 32: 0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421
 ETH: RECV:       ]
 ETH: RECV:     ]
 ETH: RECV:   ]
 ETH: RECV: ]


 // A Parity 'Header Proof' - for block 1; final I40 is { hash, totalDifficulty }
 ETH: LES: Send: [ PIP,   Headers Proof ] =>       127.0.0.1
 ETH: FOOX: L 17: [
 ETH: FOOX:   I 32: 0x42e9f16674b57980d3e657390d0f64c2060a0c0000f2a7675d313d0ea1fe5c17
 ETH: FOOX:   I 32: 0x5ac1707c14c15b3b984a4d0c10fdebef16d31098b394452415ae8b3cc67c6313
 ETH: FOOX:   I 32: 0x1fc07f6ec3a5926b306a43140eb8d54a4e287228cedec15936b27ad77e60bb04
 ETH: FOOX:   I 32: 0x64c6dfce6ffc9bd6fdbcc1cc61bd26ae6fade0526b8118f39c7a2a90e730342c
 ETH: FOOX:   I 32: 0x08e6df959ec5870b0d6d0027dfe497a15de4e83b6bec33fffd2c26e069cf3cc8
 ETH: FOOX:   I 32: 0xd8d1245ff88b155755f3e6528f3850b94ac283f75eec9739edfef9eb6d301970
 ETH: FOOX:   I 32: 0x24f3745f31f6deaed927993d333cf051d3a2c6c2e13c86a755c61f55c1151a40
 ETH: FOOX:   I 32: 0xb4bb66c1ad532011950cbe833a5ea12f726f9a2f11c57d6b9cd2a84bd0a8c36c
 ETH: FOOX:   I 32: 0xba62252ec55c99cdbdc6f576e6e439584cca6377e751634d491713cea924464d
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I  0: 0x
 ETH: FOOX: ]
 ETH: FOOX: L 17: [
 ETH: FOOX:   I  0: 0x
 ETH: FOOX:   I 32: 0x93322c467bf396006f3b161c8a21faf0e00e50f5ac35509909cc17bd85cde0ce
 ETH: FOOX:   I 32: 0x07ff24d776f04ba0f19b341d8e340d448bdd904694a28815439480d85edb0d09
 ETH: FOOX:   I 32: 0x432ea5591b2a0caa79c18c26ea71f52fcbef1c550faa6ef1229ed287bc77197f
 ETH: FOOX:   I 32: 0x21154043173bbabb8cb62fce4bb8a5d6542740aa10cffae70ec429b9a06c4e64
 ETH: FOOX:   I 32: 0x21d4ab9191f31bcba179e09f9ddddd6dbce0b5d13c902ce0dd9378de507113dc
 ETH: FOOX:   I 32: 0xbeb321608575d939442596f7a56f6e7c1d08a0d7595cb76a5014f92a7980d168
 ETH: FOOX:   I 32: 0x2d00dcea0ef53c662163da7d28d46d4aa3a06c7a64a4cecdceccb0d8e2ae6b58
 ETH: FOOX:   I 32: 0x65862ba895316f6c4f086f538b7f4d7a41c552a977d6be4cc749650683a70d7d
 ETH: FOOX:   I 32: 0xa90d17716cb50b1e873b3197a63079616d1ebdbc919b6987d8ad739c0b0a7d77
 ETH: FOOX:   I 32: 0xb6cf627115b49cbc1013c4282b6e461bf12449c1fdadf6370c7c1b50caa35d0a
 ETH: FOOX:   I 32: 0x1d8a109ecdd3c9601aa72f7334fc99f90457fbf920749951901913477bb535e0
 ETH: FOOX:   I 32: 0xd898872aa68c6e9ff48cd49f4d33189a4cff460065f0d1f1131125d640946b05
 ETH: FOOX:   I 32: 0x4bd3c6211cc0884ca8932a733541c8216cccf02ae9938cac60ed5472b97832be
 ETH: FOOX:   I 32: 0x2a83819af52f371a2b9a458dae4c6d5655eac5ff2e368715dd98a43699172e02
 ETH: FOOX:   I 32: 0x3a08d8b339727ffdfe0f377aaa394e136d2377549bf0f602eaae6a2caba7efef
 ETH: FOOX:   I  0: 0x
 ETH: FOOX: ]
 ETH: FOOX: L  2: [
 ETH: FOOX:   I  1: 0x20
 ETH: FOOX:   I 40: 0xe7a088e96d4537bea4d9c05d12549907b32561d3bf31f45aae734cdc119f13406cb68507ff800000
 ETH: FOOX: ]

*/
