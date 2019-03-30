//
//  BREthereumMPT.h
//  Core
//
//  Created by Ed Gamble on 8/21/18.
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
