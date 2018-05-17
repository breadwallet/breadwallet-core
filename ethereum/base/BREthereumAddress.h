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

extern BREthereumAddress
addressRawCreate (const char *address);

/**
 * Create an EtherumAddress from a `key` - a BRKey that already has the PubKey provided!
 */
extern BREthereumAddress
addressRawCreateKey (const BRKey *keyWithPubKeyProvided);

extern char *
addressRawGetEncodedString (BREthereumAddress address, int useChecksum);

extern BREthereumAddress
addressRawRlpDecode (BRRlpItem item,
                     BRRlpCoder coder);

extern BRRlpItem
addressRawRlpEncode(BREthereumAddress address,
                    BRRlpCoder coder);


//
// Address
//

/**
 *
 */
typedef struct BREthereumEncodedAddressRecord *BREthereumEncodedAddress;

/**
 * Create an address from the external representation of an address.  The provided address *must*
 * include a prefix of "Ox" and pass the validateAddressString() function; otherwise NULL is
 * returned.
 *
 * @param string
 * @return
 */
extern BREthereumEncodedAddress
createAddress (const char *string);

extern BREthereumEncodedAddress
createAddressDerived (const BRKey *key, uint32_t index);

/**
 * Validate `string` as an Ethereum address.  The validation is minimal - based solely on the
 * `string` content.  Said another way, the Ethereum Network is not used for validation.
 *
 * At a minimum `string` must start with "0x", have a total of 42 characters and by a 'hex' string
 * (as if a result of encodeHex(); containing characters [0-9,a-f])
 *
 * @param string
 * @return
 */
extern BREthereumBoolean
validateAddressString(const char *string);

extern uint32_t
addressGetIndex (BREthereumEncodedAddress address);
    
extern uint64_t
addressGetNonce(BREthereumEncodedAddress address);

extern void
addressFree (BREthereumEncodedAddress address);

extern BREthereumBoolean
addressHasString (BREthereumEncodedAddress address,
                  const char *string);

extern BREthereumBoolean
addressEqual (BREthereumEncodedAddress a1, BREthereumEncodedAddress a2);

/**
 * Returns a string representation of the address, newly allocated.  YOU OWN THIS.
 */
extern char *
addressAsString (BREthereumEncodedAddress address);

extern BRKey
addressGetPublicKey (BREthereumEncodedAddress address);

#if defined (DEBUG)
extern const char *
addressPublicKeyAsString (BREthereumEncodedAddress address, int compressed);
#endif

extern BRRlpItem
addressRlpEncode (BREthereumEncodedAddress address, BRRlpCoder coder);

extern BREthereumEncodedAddress
addressRlpDecode (BRRlpItem item, BRRlpCoder coder);

extern const BREthereumEncodedAddress emptyAddress;

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Address_H */
