//
//  BREthereumAddress.h
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
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

#ifndef BR_Ethereum_Address_H
#define BR_Ethereum_Address_H

#include <stdlib.h>
#include "BRKey.h"
#include "../rlp/BRRlp.h"
#include "BREthereumLogic.h"
#include "BREthereumHash.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Address Raw (QUASI-INTERNAL - currently used for Block/Log Encoding/Decoding
//
typedef struct {
    uint8_t bytes[20];
} BREthereumAddress;

#define EMPTY_ADDRESS_INIT   { \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0 \
}

#define ADDRESS_INIT(s) ((BREthereumAddress) { .bytes = {\
(_hexu((s)[ 0]) << 4) | _hexu((s)[ 1]), (_hexu((s)[ 2]) << 4) | _hexu((s)[ 3]),\
(_hexu((s)[ 4]) << 4) | _hexu((s)[ 5]), (_hexu((s)[ 6]) << 4) | _hexu((s)[ 7]),\
(_hexu((s)[ 8]) << 4) | _hexu((s)[ 9]), (_hexu((s)[10]) << 4) | _hexu((s)[11]),\
(_hexu((s)[12]) << 4) | _hexu((s)[13]), (_hexu((s)[14]) << 4) | _hexu((s)[15]),\
(_hexu((s)[16]) << 4) | _hexu((s)[17]), (_hexu((s)[18]) << 4) | _hexu((s)[19]),\
(_hexu((s)[20]) << 4) | _hexu((s)[21]), (_hexu((s)[22]) << 4) | _hexu((s)[23]),\
(_hexu((s)[24]) << 4) | _hexu((s)[25]), (_hexu((s)[26]) << 4) | _hexu((s)[27]),\
(_hexu((s)[28]) << 4) | _hexu((s)[29]), (_hexu((s)[30]) << 4) | _hexu((s)[31]),\
(_hexu((s)[32]) << 4) | _hexu((s)[33]), (_hexu((s)[34]) << 4) | _hexu((s)[35]),\
(_hexu((s)[36]) << 4) | _hexu((s)[37]), (_hexu((s)[38]) << 4) | _hexu((s)[39]) } })

extern BREthereumAddress
addressCreate (const char *address);

/**
 * Create an EtherumAddress from a `key` - a BRKey that already has the PubKey provided!
 */
extern BREthereumAddress
addressCreateKey (const BRKey *keyWithPubKeyProvided);

extern char *
addressGetEncodedString (BREthereumAddress address, int useChecksum);

extern BREthereumHash
addressGetHash (BREthereumAddress address);

extern BREthereumAddress
addressRlpDecode (BRRlpItem item,
                     BRRlpCoder coder);

extern BRRlpItem
addressRlpEncode(BREthereumAddress address,
                    BRRlpCoder coder);

extern BREthereumBoolean
addressEqual (BREthereumAddress address1,
                 BREthereumAddress address2);

static inline int
addressHashValue (BREthereumAddress address) {
    return ((UInt160 *) &address)->u32[0];
}

static inline int
addressHashEqual (BREthereumAddress address1,
                     BREthereumAddress address2) {
    return 0 == memcmp (address1.bytes, address2.bytes, 20);
}

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Address_H */
