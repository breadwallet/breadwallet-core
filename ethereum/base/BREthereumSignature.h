//
//  BREthereumSignature.h
//  BRCore
//
//  Created by Ed Gamble on 5/17/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Signature_H
#define BR_Ethereum_Signature_H

#include <stdlib.h>
#include "BREthereumLogic.h"
#include "BREthereumAddress.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Signature Type is an enumeration for the types of ethereum signatures.  There
 * are two types: one for signing transactions and one for signing P2P messages, it seems.  An
 * ethereum signature is 'recoverable' in that given the signature and the data one can recover
 * the public key (corresponding to the private key used for signing) and then from the public key
 * one can recover the address.
 */
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
 * An Ethereum Signature is the result of crytographically signing an arbitary piece of data.
 *
 * Commonly used for signing a transaction for submission to the Ehterehm P2P network.  In this
 * case the transaction is RLP encoded *without* the signature, the unsigned RLP data is signed,
 * and then the transaction is RLP encoded *with* the signature.
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

extern void
signatureClear (BREthereumSignature *s,
                BREthereumSignatureType type);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Signature_H */
