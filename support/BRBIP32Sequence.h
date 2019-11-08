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
#include "BRInt.h"
#include <stdarg.h>
#include <stddef.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// BIP32 is a scheme for deriving chains of addresses from a seed value
// https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki

#define BIP32_HARD                  0x80000000

// Each chain has a standard gap limit, as well as an extended one. The extended
// gap limit is used when constructing the P2P bloom filter, rather than the
// standard one, as this construction is an "expensive" operation (requiring
// a pause in syncing). Thus, to minimize this cost, a larger than standard
// chain length is used to construct addresses for the filter's construction.
//
// A consequence of this (as found in CORE-724) is that the wallet's chains
// need to be initialized with the extended length values. This is to avoid
// the case where a wallet is being restored (i.e. synced from its key date,
// with no blocks/transactions available initially) and a pending transaction
// is reported from the mempool (based on the larger bloom filter address
// chains). If the wallet were to be reconstructed with only that pending
// transaction loaded from the persistent store, the logic in `BRWalletNew`
// would not recognize the transaction as belonging to the wallet, assuming
// that the addresses involved were beyond the standard gap limit, and the
// function would return NULL.

#define SEQUENCE_EXTERNAL_CHAIN              0
#define SEQUENCE_INTERNAL_CHAIN              1

#define SEQUENCE_GAP_LIMIT_EXTERNAL          10
#define SEQUENCE_GAP_LIMIT_EXTERNAL_EXTENDED (SEQUENCE_GAP_LIMIT_EXTERNAL + 100)

#define SEQUENCE_GAP_LIMIT_INTERNAL          5
#define SEQUENCE_GAP_LIMIT_INTERNAL_EXTENDED (SEQUENCE_GAP_LIMIT_INTERNAL + 100)

typedef struct {
    uint32_t fingerPrint;
    UInt256 chainCode;
    uint8_t pubKey[33];
} BRMasterPubKey;

#define BR_MASTER_PUBKEY_NONE ((const BRMasterPubKey) { 0, UINT256_ZERO, \
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } })

// returns the master public key for the default BIP32 wallet layout - derivation path N(m/0H)
BRMasterPubKey BRBIP32MasterPubKey(const void *seed, size_t seedLen);

// writes the public key for path N(m/0H/chain/index) to pubKey
// returns number of bytes written, or pubKeyLen needed if pubKey is NULL
size_t BRBIP32PubKey(uint8_t *pubKey, size_t pubKeyLen, BRMasterPubKey mpk, uint32_t chain, uint32_t index);

// sets the private key for path m/0H/chain/index to key
void BRBIP32PrivKey(BRKey *key, const void *seed, size_t seedLen, uint32_t chain, uint32_t index);

// sets the private key for path m/0H/chain/index to each element in keys
void BRBIP32PrivKeyList(BRKey keys[], size_t keysCount, const void *seed, size_t seedLen, uint32_t chain,
                        const uint32_t indexes[]);
    
// sets the private key for the specified path to key
// depth is the number of arguments used to specify the path
void BRBIP32PrivKeyPath(BRKey *key, const void *seed, size_t seedLen, int depth, ...);

// sets the private key for the path specified by vlist to key
// depth is the number of arguments in vlist
void BRBIP32vPrivKeyPath(BRKey *key, const void *seed, size_t seedLen, int depth, va_list vlist);

// writes the base58check encoded serialized master private key (xprv) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BRBIP32SerializeMasterPrivKey(char *str, size_t strLen, const void *seed, size_t seedLen);

// writes the base58check encoded serialized master public key (xpub) to str
// returns number of bytes written including NULL terminator, or strLen needed if str is NULL
size_t BRBIP32SerializeMasterPubKey(char *str, size_t strLen, BRMasterPubKey mpk);

// returns a master public key give a base58check encoded serialized master public key (xpub)
BRMasterPubKey BRBIP32ParseMasterPubKey(const char *str);

// key used for authenticated API calls, i.e. bitauth: https://github.com/bitpay/bitauth - path m/1H/0
void BRBIP32APIAuthKey(BRKey *key, const void *seed, size_t seedLen);

// key used for BitID: https://github.com/bitid/bitid/blob/master/BIP_draft.md
void BRBIP32BitIDKey(BRKey *key, const void *seed, size_t seedLen, uint32_t index, const char *uri);

#ifdef __cplusplus
}
#endif

#endif // BRBIP32Sequence_h
