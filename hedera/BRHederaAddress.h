//
//  BRHederaAddress.h
//  Core
//
//  Created by Carl Cherry on Oct. 23, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRHederaAddress_h
#define BRHederaAddress_h

#include "support/BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRHederaAddressRecord *BRHederaAddress;

/**
 * Get the hedera address string representation of the address
 *
 * @param address   - a BRHederaAddress
 *
 * @return pointer to allocated buffer holding the null terminated string
 */
extern char * // Caller must free using the "free" function
hederaAddressAsString (BRHederaAddress address);

/**
 * Create a hedera address from a valid hedera address string
 *
 * @param address   - hedera address string in the "r..." format
 *
 * @return address  - a BRHederaAddress object
 */
extern BRHederaAddress
hederaAddressCreateFromString(const char * hederaAddressString);

/**
 * Free the memory associated with a BRHederaAddress
 *
 * @param address   - a BRHederaAddress
 *
 * @return void
 */
extern void
hederaAddressFree (BRHederaAddress address);

/**
 * Check is this address is the
 *
 * @param address   - a BRHederaAddress
 *
 * @return 1 if this is the "Fee" address, 0 if not
 */
extern int
hederaAddressIsFeeAddress (BRHederaAddress address);

/**
 * Get the Hedera address field values
 *
 * @param address    - a BRHederaAddress
 *
 * @return int64_t value
 */
int64_t hederaAddressGetShard (BRHederaAddress address);
int64_t hederaAddressGetRealm (BRHederaAddress address);
int64_t hederaAddressGetAccount (BRHederaAddress address);

/**
 * Copy a BRHederaAddress
 *
 * @param address   - a BRHederaAddress
 *
 * @return copy     - an exact copy of the specified address
 */
extern BRHederaAddress hederaAddressClone (BRHederaAddress address);

/**
 * Compare 2 hedera addresses
 *
 * @param a1  first address
 * @param a2  second address
 *
 * @return 1 - if addresses are equal
 *         0 - if not equal
 */
extern int // 1 if equal
hederaAddressEqual (BRHederaAddress a1, BRHederaAddress a2);

/**
 * Serialize an Hedera account
 *
 * @param address           the hedera address to serialize
 * @param sizeOfBytes   pointer to size_t to hold size of serialized bytes
 *
 * @return bytes       pointer to buffer holding serialized bytes
 */
extern uint8_t * hederaAddressSerialize (BRHederaAddress address, size_t * sizeOfBytes);

/**
* Create an Hedera account from a byte stream
*
* @param bytes              bytes serialized by hederAddressSerialize
* @param sizeOfBytes   the size of the byte array
*
* @return address          pointer to an Heder address
*/
extern BRHederaAddress hederaAddressCreateFromBytes (uint8_t * bytes, size_t sizeOfBytes);

#ifdef __cplusplus
}
#endif

#endif /* BRHederaAddress_h */

