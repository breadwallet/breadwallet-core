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
    SIGNATURE_TYPE_FOO,
    SIGNATURE_TYPE_RECOVERABLE
} BREthereumSignatureType;

typedef struct {
    BREthereumSignatureType type;
    union {
        struct {
            int ignore;
        } foo;

        struct {
            uint8_t v;
            uint8_t r[32];
            uint8_t s[32];
        } recoverable;
    } sig;
} BREthereumSignature;

extern BREthereumSignature
signatureCreate (BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount,
                 BRKey privateKeyUncompressed);

extern BREthereumEncodedAddress
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
