//
//  BREthereumAccountState.c
//  BRCore
//
//  Created by Ed Gamble on 5/15/18.
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
#include <assert.h>
#include "BREthereumAccountState.h"

extern BREthereumAccountState
accountStateCreateEmpty (void) {
    BREthereumAccountState state = EMPTY_ACCOUNT_STATE_INIT;
    return state;
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

    items[0] = rlpEncodeItemUInt64(coder, state.nonce, 0);
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

    state.nonce = rlpDecodeItemUInt64(coder, items[0], 0);
    state.balance = etherRlpDecode(items[1], coder);
    state.storageRoot = hashRlpDecode(items[2], coder);
    state.codeHash = hashRlpDecode(items[3], coder);

    return state;
}

