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
#include "BRRipplePrivateStructs.h"
#include "BRCrypto.h"

extern BRRippleSignature
signBytes (BRKey *key, uint8_t *bytes, size_t bytesCount)
{
    BRRippleSignature sig = calloc(1,sizeof(BRRippleSignatureRecord));

    // Before hashing the transaction - append the prefix
    uint8_t HASH_TX_SIGN[4] = { 0x53, 0x54, 0x58, 0x00 }; // 0x53545800  # 'STX'
    UInt256 messageDigest;
    uint8_t bytes_to_hash[4 + bytesCount];
    memcpy(bytes_to_hash, HASH_TX_SIGN, 4);
    memcpy(&bytes_to_hash[4], bytes, bytesCount);

    // Create a sha512 hash and only use the first 32 bytes
    uint8_t hash[64];
    BRSHA512(hash, bytes_to_hash, bytesCount + 4);
    memcpy(messageDigest.u8, hash, 32);

    // BRKeySign (not sure if this is a good name) but it signs the key and does DER encoding.
    // TODO - figure out what the max size the buffer needs to be, currently hard coded to 256 bytes
    // but all I have seen so far is either 70 or 71 bytes
    sig->sig_length = (int)BRKeySign(key, sig->signature, (int)sizeof(sig->signature), messageDigest);
    
    return sig;
}

extern void rippleSignatureDelete(BRRippleSignature signature)
{
    assert(signature);
    free(signature);
}

