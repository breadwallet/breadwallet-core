//
//  BRRippleAddress.h
//  Core
//
//  Created by Carl Cherry on Oct. 21, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRippleAddress_h
#define BRRippleAddress_h

#include "support/BRKey.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BRRippleAddressRecord *BRRippleAddress;

/**
 * Get the ripple address string representation of the address
 *
 * @param address   - a BRRippleAddress
 *
 * @return pointer to allocated buffer holding the null terminated string
 */
extern char * // Caller must free using the "free" function
rippleAddressAsString (BRRippleAddress address);

/**
 * Create a ripple address from a valid public key
 *
 * @param key   - a BRKey with the public key set
 *
 * @return address  - a BRRippleAddress object
 */
extern BRRippleAddress // caller must free memory with rippleAddressFree
rippleAddressCreateFromKey (BRKey *publicKey);

/**
 * Create a ripple address from a valid ripple address string
 *
 * @param address   - ripple address string in the "r..." format
 *
 * @return address  - a BRRippleAddress object
 */
extern BRRippleAddress
rippleAddressCreateFromString(const char * rippleAddressString);

/**
 * Create a ripple address from the raw bytes (20 for ripple)
 *
 * @param buffer     - raw address bytes
 * @param bufferSize - size of the buffer, must be 20
 *
 * @return address  - a BRRippleAddress object
 */
extern BRRippleAddress // caller must free memory with rippleAddressFree
rippleAddressCreateFromBytes (uint8_t * buffer, int bufferSize);

/**
 * Free the memory associated with a BRRippleAddress
 *
 * @param address   - a BRRippleAddress
 *
 * @return void
 */
extern void
rippleAddressFree (BRRippleAddress address);

/**
 * Check is this address is the
 *
 * @param address   - a BRRippleAddress
 *
 * @return 1 if this is the "Fee" address, 0 if not
 */
extern int
rippleAddressIsFeeAddress (BRRippleAddress address);

/**
 * Get the size of the raw bytes for this address
 *
 * @param address   - a BRRippleAddress
 *
 * @return size of the raw bytes
 */
extern int
rippleAddressGetRawSize (BRRippleAddress address);

/**
 * Get the raw bytes for this address
 *
 * @param address    - a BRRippleAddress
 * @param buffer     - a buffer to hold the raw bytes
 * @param bufferSize - size of the buffer, obtained via rippleAddressGetRawSize
 *
 * @return void
 */
extern void rippleAddressGetRawBytes (BRRippleAddress address, uint8_t *buffer, int bufferSize);

/**
 * Copy a BRRippleAddress
 *
 * @param address   - a BRRippleAddress
 *
 * @return copy     - an exact copy of the specified address
 */
extern BRRippleAddress rippleAddressClone (BRRippleAddress address);

/**
 * Compare 2 ripple addresses
 *
 * @param a1  first address
 * @param a2  second address
 *
 * @return 1 - if addresses are equal
 *         0 - if not equal
 */
extern int // 1 if equal
rippleAddressEqual (BRRippleAddress a1, BRRippleAddress a2);

#ifdef __cplusplus
}
#endif

#endif /* BRRippleAddress_h */
