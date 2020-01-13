//
//  BRRippleSignature.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_signature_h
#define BRRipple_signature_h

#include "BRRippleBase.h"

/**
 * Sign the bytes with the master private key
 *
 * @param key         private key for this account
 * @param bytes       bytes to sign (a serialized transaction)
 * @param bytesCount  number of bytes to sign
 *
 * @return signature  the structure holding the signature
 */
extern BRRippleSignature
signBytes (BRKey *key, uint8_t *bytes, size_t bytesCount);

extern void rippleSignatureDelete (BRRippleSignature signature);
#endif
