//
//  BREthereumAccountState.h
//  BRCore
//
//  Created by Ed Gamble on 5/15/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_AccountState_H
#define BR_Ethereum_AccountState_H

#include "ethereum/base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Account state.
 *
 * From the Etheruem specification:  The account state, σ[a], comprises the following four
 * fields: {nonce, balance, storageRoot, codeHash}
 */
typedef struct {
    /**
     * A scalar value equal to the number of trans- actions sent from this address or, in the
     * case of accounts with associated code, the number of contract-creations made by this
     * account. For ac- count of address a in state σ, this would be for- mally denoted σ[a]n.
     */
    uint64_t nonce;

    /**
     * A scalar value equal to the number of Wei owned by this address. Formally denoted σ[a]b.
     */
    BREthereumEther balance;

    /**
     * A 256-bit hash of the root node of a Merkle Patricia tree that encodes the storage contents
     * of the account (a mapping between 256-bit integer values), encoded into the trie as a
     * mapping from the Keccak 256-bit hash of the 256-bit integer keys to the RLP-encoded 256-bit
     * integer values. The hash is formally denoted σ[a]s.
     */
    BREthereumHash storageRoot;

    /**
     * The hash of the EVM code of this account this is the code that gets executed should this
     * address receive a message call; it is immutable and thus, unlike all other fields, cannot
     * be changed after construction. All such code fragments are contained in the state database
     * under their corresponding hashes for later retrieval. This hash is formally denoted σ[a]c,
     * and thus the code may be denoted as b, given that KEC(b) = σ[a]c.
     */
    BREthereumHash codeHash;
} BREthereumAccountState;

#define EMPTY_ACCOUNT_STATE_INIT    ((BREthereumAccountState) { \
    0, EMPTY_ETHER_INIT, EMPTY_HASH_INIT, EMPTY_HASH_INIT \
})

extern BREthereumAccountState
accountStateCreate (uint64_t nonce,
                    BREthereumEther balance,
                    BREthereumHash storageRoot,
                    BREthereumHash codeHash);
    
extern BREthereumAccountState
accountStateCreateEmpty (void);
    
extern uint64_t
accountStateGetNonce (BREthereumAccountState state);

extern BREthereumEther
accountStateGetBalance (BREthereumAccountState state);

extern BREthereumHash
accountStateGetStorageRoot (BREthereumAccountState state);

extern BREthereumHash
accountStateGetCodeHash (BREthereumAccountState state);

extern BREthereumBoolean
accountStateEqual (BREthereumAccountState s1,
                   BREthereumAccountState s2);
    
extern BRRlpItem
accountStateRlpEncode (BREthereumAccountState state, BRRlpCoder coder);

extern BREthereumAccountState
accountStateRlpDecode (BRRlpItem item, BRRlpCoder coder);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_AccountState_H */
