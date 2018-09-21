//
//  BREthereumSignature.h
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

#ifndef BR_Ethereum_Signature_H
#define BR_Ethereum_Signature_H

#include <stdlib.h>
#include "BREthereumLogic.h"
#include "BREthereumAddress.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Signature
//

typedef enum {
    /**
     * A 'VRS' signature suitable for Ethereum Transaction signing.  Modifies the 'v' field with
     * a constant offset (0x1b == 27) and a compressed offset (4 if compressed, 0 otherwise).
     */
    SIGNATURE_TYPE_RECOVERABLE_VRS_EIP,

    /**
     * A 'RSV' signature suitable for 'standard' (non-transaction) signing with a recoverable
     * signature.  Used for Ethereum messaging (P2P, DIS, LES, etc).
     */
    SIGNATURE_TYPE_RECOVERABLE_RSV
} BREthereumSignatureType;


/**
 * A VRS signature - with a Network chainID encoded into 'v'
 */
typedef struct {
    uint8_t v;
    uint8_t r[32];
    uint8_t s[32];
} BREthereumSignatureVRS;


/**
 * A RSV signature - the standard recoverable signature
 */
typedef struct {
    uint8_t r[32];
    uint8_t s[32];
    uint8_t v;
} BREthereumSignatureRSV;


/**
 * An Ethereum Signature - one of the two types: VRS_EIP or RSV
 */
typedef struct {
    BREthereumSignatureType type;
    union {
        BREthereumSignatureVRS vrs;
        BREthereumSignatureRSV rsv;
    } sig;
} BREthereumSignature;

extern BREthereumSignature
signatureCreate (BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 BRKey privateKeyUncompressed);

extern BREthereumAddress
signatureExtractAddress (const BREthereumSignature signature,
                         const uint8_t *bytes,
                         size_t bytesCount,
                         int *success);

extern BREthereumBoolean
signatureEqual (BREthereumSignature s1, BREthereumSignature s2);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Signature_H */
