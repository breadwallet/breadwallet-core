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
        case SIGNATURE_TYPE_RECOVERABLE_VRS_EIP: {
            // Determine the signature length
            size_t signatureLen = BRKeyCompactSign (&privateKeyUncompressed,
                                                    NULL, 0,
                                                    messageDigest);

            // Fill the signature
            uint8_t signatureBytes[signatureLen];
            signatureLen = BRKeyCompactSign (&privateKeyUncompressed,
                                             signatureBytes, signatureLen,
                                             messageDigest);
            assert (65 == signatureLen);

            // The actual 'signature' is one byte added to secp256k1_ecdsa_recoverable_signature
            // and secp256k1_ecdsa_recoverable_signature is 64 bytes as {r[32], s32]}

            // Extract V, R, and S
            signature.sig.vrs.v = signatureBytes[0];
            memcpy(signature.sig.vrs.r, &signatureBytes[ 1], 32);
            memcpy(signature.sig.vrs.s, &signatureBytes[33], 32);

            // TODO: Confirm signature
            // assigns pubKey recovered from compactSig to key and returns true on success
            // int BRKeyRecoverPubKey(BRKey *key, UInt256 md, const void *compactSig, size_t sigLen)

            break;
        }

        case SIGNATURE_TYPE_RECOVERABLE_RSV: {
            // Determine the signature length
            size_t signatureLen = BRKeyCompactSignEthereum (&privateKeyUncompressed,
                                                            NULL, 0,
                                                            messageDigest);

            // Fill the signature
            uint8_t signatureBytes[signatureLen];
            signatureLen = BRKeyCompactSignEthereum (&privateKeyUncompressed,
                                                     signatureBytes, signatureLen,
                                                     messageDigest);
            assert (65 == signatureLen);

            // The actual 'signature' is one byte added to secp256k1_ecdsa_recoverable_signature
            // and secp256k1_ecdsa_recoverable_signature is 64 bytes as {r[32], s32]}

            // Extract V, R, and S
            memcpy(signature.sig.rsv.r, &signatureBytes[ 0], 32);
            memcpy(signature.sig.rsv.s, &signatureBytes[32], 32);
            signature.sig.rsv.v = signatureBytes[64];

            break;
        }
    }

    return signature;
}

extern BREthereumBoolean
signatureEqual (BREthereumSignature s1, BREthereumSignature s2) {
    return (s1.type == s2.type &&
            s1.sig.vrs.v == s2.sig.vrs.v &&
            0 == memcmp (s1.sig.vrs.r, s2.sig.vrs.r, 32) &&
            0 == memcmp (s1.sig.vrs.s, s2.sig.vrs.s, 32)
            ? ETHEREUM_BOOLEAN_TRUE
            : ETHEREUM_BOOLEAN_FALSE);
}

extern BREthereumAddress
signatureExtractAddress(const BREthereumSignature signature,
                        const uint8_t *bytes,
                        size_t bytesCount,
                        int *success) {
    assert (NULL != success);

    UInt256 digest;
    BRKeccak256 (&digest, bytes, bytesCount);

    BRKey key;

    switch (signature.type) {
        case SIGNATURE_TYPE_RECOVERABLE_VRS_EIP:
            *success = BRKeyRecoverPubKey (&key, digest,
                                           &signature.sig.vrs,
                                           sizeof (signature.sig.vrs));
            break;
        case SIGNATURE_TYPE_RECOVERABLE_RSV:
            *success = BRKeyRecoverPubKeyEthereum (&key, digest,
                                                   &signature.sig.rsv,
                                                   sizeof (signature.sig.rsv));
            break;
    }

    return (0 == *success
            ? (BREthereumAddress) EMPTY_ADDRESS_INIT
            : addressCreateKey(&key));
}
