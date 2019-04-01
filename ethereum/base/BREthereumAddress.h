//
//  BREthereumAddress.h
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Address_H
#define BR_Ethereum_Address_H

#include <stdlib.h>
#include "support/BRKey.h"
#include "ethereum/rlp/BRRlp.h"
#include "BREthereumLogic.h"
#include "BREthereumHash.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADDRESS_BYTES   (20)

/**
 * An Ethereum Address
 */
typedef struct {
    uint8_t bytes[ADDRESS_BYTES];
} BREthereumAddress;

#define EMPTY_ADDRESS_INIT   ((const BREthereumAddress) { \
0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0 \
})

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

extern BREthereumBoolean
addressValidateString(const char *string);

/**
 * Create an EtherumAddress from a `key` - a BRKey that already has the PubKey provided!
 */
extern BREthereumAddress
addressCreateKey (const BRKey *keyWithPubKeyProvided);

#define ADDRESS_ENCODED_CHARS    (2*ADDRESS_BYTES + 2 + 1)  // "0x" prefaced

/**
 * Return a string encoding of `address`.  You own the returned string and must free it.
 *
 * @param address
 * @param useChecksum if true(1) return an address with checksummed characters.
 * 
 * @return newly allocated memory of char*
 */
extern char *
addressGetEncodedString (BREthereumAddress address, int useChecksum);


/**
 * Fill `string` with the string encoding of `address`.  The `string` must be 43 characters in
 * length and will be `0x` prefaces.
 *
 * @param address address to encode
 * @param useChecksum if true(1) fill in string with checksummed characters
 * @param string target to fill
 */
extern void
addressFillEncodedString (BREthereumAddress address,
                          int useChecksum,
                          char *string);

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
