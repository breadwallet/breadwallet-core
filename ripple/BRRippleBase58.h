//
//  BRRippleBase58.h
//  Core
//
//  Created by Carl Cherry on May 3, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_base58_h
#define BRRipple_base58_h

#include <stdlib.h>
#include "BRRippleBase.h"

/**
 * Convert a ripple address string to the account ID bytes
 *
 * @param rippleAddressString   i.e. r41vZ8....
 * @param address               pointer to a BRRippleAddress
 *
 * @return number of bytes written to the address
 */
extern int rippleAddressStringToAddress(const char* rippleAddressString, BRRippleAddress * address);


#endif
