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

#define HEDERA_ADDRESS_SERIALIZED_SIZE 24

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

extern BRHederaAddress
hederaAddressCreate(int64_t shard, int64_t realm, int64_t account_num);

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
 *   Serialize an Hedera address for storing
 * @param address - the hedera address to serialize
 * @param size_t* pointer to hold the size of the serialized bytes
 *
 * @return pointer to buffer holding bytes
 */
extern void
hederaAddressSerialize(BRHederaAddress address, uint8_t * buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif /* BRHederaAddress_h */

