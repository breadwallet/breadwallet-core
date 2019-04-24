//
//  BRRippleSerialize.c
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "BRRipple.h"
#include "BRRippleBase.h"
#include "BRCrypto.h"

extern BRRippleSignature
signBytes (BRKey *key, uint8_t *bytes, size_t bytesCount)
{
    BRRippleSignature sig = calloc(1,sizeof(BRRippleSignatureRecord));

    // Hash with the required Keccak-256
    // TODO (1) - is this the correct thing to do for Ripple transaction???
    // documentation is light on the algorithm
    // TODO (2) - in some of the files that show signing some HashPrefix is
    // appended at the beginning of the bytes - in our case if needed it would
    // be { 'T', 'X', 'N', 0x00 }
    UInt256 messageDigest;
    BRKeccak256(&messageDigest, bytes, bytesCount);

    size_t signatureLen = 65;
    signatureLen = BRKeyCompactSign (key, sig->signature, signatureLen, messageDigest);
    assert (65 == signatureLen);

    return sig;
}

extern void rippleSignatureDelete(BRRippleSignature signature)
{
    assert(signature);
    free(signature);
}

