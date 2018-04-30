//
//  BREthereumSignature.c
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

#include <assert.h>
#include "BRCrypto.h"
#include "BREthereumSignature.h"

//
// Signature
//
extern BREthereumSignature
signatureCreate(BREthereumSignatureType type,
                uint8_t *bytes,
                size_t bytesCount,
                BRKey privateKeyUncompressed) {
    BREthereumSignature signature;

    // Save the type.
    signature.type = type;

    // Hash with the required Keccak-256
    UInt256 messageDigest;
    BRKeccak256(&messageDigest, bytes, bytesCount);

    switch (type) {
        case SIGNATURE_TYPE_FOO:
            break;

        case SIGNATURE_TYPE_RECOVERABLE: {
            // Determine the signature length
            size_t signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                                   NULL, 0,
                                                   messageDigest);

            // Fill the signature
            uint8_t signatureBytes[signatureLen];
            signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                            signatureBytes, signatureLen,
                                            messageDigest);
            assert (65 == signatureLen);

            // The actual 'signature' is one byte added to secp256k1_ecdsa_recoverable_signature
            // and secp256k1_ecdsa_recoverable_signature is 64 bytes as {r[32], s32]}

            // Extract V, R, and S
            signature.sig.recoverable.v = signatureBytes[0];
            memcpy(signature.sig.recoverable.r, &signatureBytes[ 1], 32);
            memcpy(signature.sig.recoverable.s, &signatureBytes[33], 32);

            // TODO: Confirm signature
            // assigns pubKey recovered from compactSig to key and returns true on success
            // int BRKeyRecoverPubKey(BRKey *key, UInt256 md, const void *compactSig, size_t sigLen)

            break;
        }
    }

    return signature;
}

extern BREthereumBoolean
signatureEqual (BREthereumSignature s1, BREthereumSignature s2) {
    return (s1.sig.recoverable.v == s2.sig.recoverable.v &&
            0 == memcmp (s1.sig.recoverable.r, s2.sig.recoverable.r, 32) &&
            0 == memcmp (s1.sig.recoverable.s, s2.sig.recoverable.s, 32)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumEncodedAddress
signatureExtractAddress (const BREthereumSignature signature,
                         const uint8_t *bytes,
                         size_t bytesCount,
                         int *success) {
    assert (NULL != success);

    if (SIGNATURE_TYPE_RECOVERABLE != signature.type) {
        *success = 0;
        return emptyAddress;
    }

    UInt256 digest;
    BRKeccak256 (&digest, bytes, bytesCount);

    BRKey key;
    *success = BRKeyRecoverPubKey(&key, digest,
                                  &signature.sig.recoverable,
                                  sizeof (signature.sig.recoverable));

    if (0 == *success)
        return emptyAddress;

    return createAddressDerived (&key, 0);
}
