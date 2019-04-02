//
//  BREthereumMPT.h
//  Core
//
//  Created by Ed Gamble on 8/21/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_MPT_h
#define BR_Ethereum_MPT_h

#include "support/BRArray.h"
#include "ethereum/rlp/BRRlp.h"
#include "ethereum/base/BREthereumData.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // Leaf: A two-item structure whose first item corresponds to the nibbles in the key not
    // already accounted for by the accumulation of keys and branches traversed from the root. The
    // hex-prefix encoding method is used and the second parameter to the function is required to
    // be true.
    MPT_NODE_LEAF,
    
    // Extension: A two-item structure whose first item corresponds to a series of nibbles of size
    // greater than one that are shared by at least two distinct keys past the accumulation of the
    // keys of nibbles and the keys of branches as traversed from the root. The hex-prefix encoding
    // method is used and the second parameter to the function is required to be false.
    MPT_NODE_EXTENSION,
    
    // Branch: A 17-item structure whose first sixteen items correspond to each of the sixteen
    // possible nibble values for the keys at this point in their traversal. The 17th item is used
    // in the case of this being a terminator node and thus a key being ended at this point in
    // its traversal.
    MPT_NODE_BRANCH
} BREthereumMPTNodeType;

//
//
//
//
//
//
typedef struct BREthereumMPTNodePathRecord *BREthereumMPTNodePath;

extern void
mptNodePathRelease (BREthereumMPTNodePath path);

extern void
mptNodePathsRelease (BRArrayOf(BREthereumMPTNodePath) paths);

extern BRRlpData
mptNodePathGetValue (BREthereumMPTNodePath path,
                     BREthereumData key,
                     BREthereumBoolean *found);

extern BREthereumData
mptNodePathGetKeyFragment (BREthereumMPTNodePath path);
    
extern BREthereumBoolean
mptNodePathIsValid (BREthereumMPTNodePath path,
                    BREthereumData key);

extern BREthereumMPTNodePath
mptNodePathDecode (BRRlpItem item,
                   BRRlpCoder coder);

/**
 * Decode a MPT from an RLP item that is a RLP list of bytes.  This is unlike the above which
 * is an RLP List of RLP List of ...
 */
extern BREthereumMPTNodePath
mptNodePathDecodeFromBytes (BRRlpItem item,
                            BRRlpCoder coder);

/**
 * Create a Key Path from a value
 */
extern BREthereumData
mptKeyGetFromUInt64 (uint64_t value);

/**
 * Create a Key Path from a hash
 */
extern BREthereumData
mptKeyGetFromHash (BREthereumHash hash);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_MPT_h */
