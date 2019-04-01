//
//  BREthereumAccountState.c
//  BRCore
//
//  Created by Ed Gamble on 5/15/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdlib.h>
#include <assert.h>
#include "BREthereumAccountState.h"

extern BREthereumAccountState
accountStateCreateEmpty (void) {
    return EMPTY_ACCOUNT_STATE_INIT;
}

extern uint64_t
accountStateGetNonce (BREthereumAccountState state) {
    return state.nonce;
}

extern BREthereumEther
accountStateGetBalance (BREthereumAccountState state) {
    return state.balance;
}

extern BREthereumHash
accountStateGetStorageRoot (BREthereumAccountState state) {
    return state.storageRoot;
}

extern BREthereumHash
accountStateGetCodeHash (BREthereumAccountState state) {
    return state.codeHash;
}

extern BREthereumBoolean
accountStateEqual (BREthereumAccountState s1,
                   BREthereumAccountState s2) {
    return AS_ETHEREUM_BOOLEAN(s1.nonce == s2.nonce &&
                               ETHEREUM_BOOLEAN_IS_TRUE(etherIsEQ(s1.balance, s2.balance)));
}

extern BREthereumAccountState
accountStateCreate (uint64_t nonce,
                    BREthereumEther balance,
                    BREthereumHash storageRoot,
                    BREthereumHash codeHash) {

    BREthereumAccountState state;

    state.nonce = nonce;
    state.balance = balance;
    state.storageRoot = storageRoot;
    state.codeHash = codeHash;

    return state;
}

extern BRRlpItem
accountStateRlpEncode(BREthereumAccountState state, BRRlpCoder coder) {
    BRRlpItem items[4];

    items[0] = rlpEncodeUInt64(coder, state.nonce, 0);
    items[1] = etherRlpEncode(state.balance, coder);
    items[2] = hashRlpEncode(state.storageRoot, coder);
    items[3] = hashRlpEncode(state.codeHash, coder);

    return rlpEncodeListItems(coder, items, 4);
}

extern BREthereumAccountState
accountStateRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    BREthereumAccountState state;

    size_t itemsCount = 0;
    const BRRlpItem *items = rlpDecodeList(coder, item, &itemsCount);
    assert (4 == itemsCount);

    state.nonce = rlpDecodeUInt64(coder, items[0], 0);
    state.balance = etherRlpDecode(items[1], coder);
    state.storageRoot = hashRlpDecode(items[2], coder);
    state.codeHash = hashRlpDecode(items[3], coder);

    return state;
}

