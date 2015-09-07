//
//  BRBIP32Sequence.h
//
//  Created by Aaron Voisine on 8/19/15.
//  Copyright (c) 2015 breadwallet LLC
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

#ifndef BRBIP32Sequence_h
#define BRBIP32Sequence_h

#include "BRKey.h"
#include "BRTypes.h"
#include <stddef.h>

// BIP32 is a scheme for deriving chains of addresses from a seed value
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki

typedef struct {
    uint32_t fingerPrint;
    UInt256 chainCode;
    BRPubKey pubKey;
} BRMasterPubKey;

#define MASTER_PUBKEY_NONE ((BRMasterPubKey) { 0, UINT256_ZERO, PUBKEY_NONE })

BRMasterPubKey BRBIP32MasterPubKey(const void *seed, size_t seedLen);
BRPubKey BRBIP32PubKey(BRMasterPubKey mpk, int internal, uint32_t index);
void BRBIP32PrivKey(UInt256 *key, const void *seed, size_t seedLen, int internal, uint32_t index);
void BRBIP32PrivKeyList(UInt256 *keys, size_t count, const void *seed, size_t seedLen, int internal,
                        const uint32_t *indexes);

size_t BRBIP32SerializeMasterPrivKey(char *s, size_t len, const void *seed, size_t seedLen);
size_t BRBIP32SerializeMasterPubKey(char *s, size_t len, BRMasterPubKey mpk);

#endif // BRBIP32Sequence_h
