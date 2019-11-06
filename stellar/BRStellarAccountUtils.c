//
//  BRStellarAccountUtils.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

//
// The following algorithms were derived from https://github.com/reverbel/seed-phrases-for-stellar
// Copyright (c) 2018 Francisco Reverbel - see license below
// 1. Generate 32-byte random seed from BIP39 paper key
// 2. Generate Stellar account address from public key
//

/*
MIT License

Copyright (c) 2018 Francisco Reverbel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "BRStellarAccountUtils.h"
#include "BRInt.h"
#include "BRBIP39Mnemonic.h"
#include "BRBIP32Sequence.h"
#include "support/BRCrypto.h"
#include "utils/crc16.h"
#include "utils/base32.h"
#include <assert.h>
#include <stdarg.h>

#define ED25519_SEED_KEY "ed25519 seed"

static BRKey deriveStellarKeyFromSeed (UInt512 seed, uint32_t index);

BRKey createStellarKeyFromPaperKey(const char* paperKey)
{
    assert(paperKey);

    // Generate the 512bit private key using a BIP39 paperKey
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paperKey, NULL); // no passphrase

    return deriveStellarKeyFromSeed(seed, 0);
}

BRKey createStellarKeyFromSeed(UInt512 seed)
{
    return deriveStellarKeyFromSeed(seed, 0);
}

extern BRStellarAddress createStellarAddressFromPublicKey(BRKey * key)
{
    BRStellarAddress address;
    
    assert(key);

    // The public key is 32 bytes, we need 3 additional bytes
    uint8_t rawBytes[35];
    rawBytes[0] = 0x30; // account prefix
    memcpy(&rawBytes[1], key->pubKey, 32); // public key bytes
    
    // Create a CRC16-XMODEM checksum and append to bytes
    uint16_t checksum = crc16((const char*)rawBytes, 33);
    rawBytes[33] = (checksum & 0x00FF);
    rawBytes[34] = (checksum & 0xFF00) >> 8;
    
    // Base32 the result
    unsigned char encoded_bytes[STELLAR_ADDRESS_BYTES + 1] = {0};
    base32_encode(rawBytes, 35, encoded_bytes);
    
    memcpy(address.bytes, encoded_bytes, sizeof(address.bytes));
    return(address);
}

BRStellarAccountID createStellarAccountIDFromStellarAddress(const char *stellarAddress)
{
    // Decode the address which should take no more than 35 bytes
    uint8_t rawBytes[35];
    base32_decode((const unsigned char*)stellarAddress, rawBytes);

    BRStellarAccountID accountID;
    accountID.accountType = PUBLIC_KEY_TYPE_ED25519;
    // Validation - the first byte should be 0x30
    if (0x30 == rawBytes[0]) {
        // We could also check last 2 bytes CRC checksum
        memcpy(accountID.accountID, &rawBytes[1], 32);
    }
    return accountID;
}

// sets the private key for the path specified by vlist to key
// depth is the number of arguments in vlist
static void _deriveKeyImpl(BRKey *key, const void *seed, size_t seedLen, int depth, va_list vlist)
{
    UInt512 I;
    UInt256 secret, chainCode;
    
    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);
    assert(depth == 3);
    
    if (key && (seed || seedLen == 0)) {
        // For the first hash the key is "ed25519 seed"
        BRHMAC(&I, BRSHA512, sizeof(UInt512), ED25519_SEED_KEY, strlen(ED25519_SEED_KEY), seed, seedLen);
        
        for (int i = 0; i < depth; i++) {
            secret = *(UInt256 *)&I;
            chainCode = *(UInt256 *)&I.u8[sizeof(UInt256)];

            // Take the result from the first hash and create some new data to hash
            // data = b'\x00' + secret + ser32(i)
            uint8_t data[37] = {0};
            memcpy(&data[1], secret.u8, 32);
            // Copy the bytes from i to the end of data
            uint32_t seq = va_arg(vlist, uint32_t);
            assert(seq & 0xF0000000); // Needs to be hardened
            data[33] = (seq & 0xFF000000) >> 24;
            data[34] = (seq & 0x00FF0000) >> 16;
            data[35] = (seq & 0x0000FF00) >> 8;
            data[36] = (seq & 0x000000FF);

            // For the remaining hashing the key is the chaincode from the previous hash
            var_clean(&I); // clean before next hash
            BRHMAC(&I, BRSHA512, sizeof(UInt512), chainCode.u8, 32, data, 37);
        }

        // We are done - take that last hash result and it becomes our secret
        key->secret = *(UInt256 *)&I;

        // Erase from memory
        var_clean(&I);
        var_clean(&secret, &chainCode);
    }
}


// sets the private key for the specified path to key
// depth is the number of arguments used to specify the path
static void _stellarDeriveKey(BRKey *key, const void *seed, size_t seedLen, int depth, ...)
{
    va_list ap;
    
    va_start(ap, depth);
    _deriveKeyImpl(key, seed, seedLen, depth, ap);
    va_end(ap);
}

static BRKey deriveStellarKeyFromSeed (UInt512 seed, uint32_t index)
{
    BRKey privateKey;
    
    // The BIP32 privateKey for m/44'/148'/%d' where %d is the key index
    // The stellar system only supports ed25519 key pairs. There are some restrictions when it comes
    // to creating key pairs for ed25519, described here:
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0005.md
    _stellarDeriveKey(&privateKey, &seed, sizeof(UInt512), 3,
                      44 | BIP32_HARD,     // purpose  : BIP-44
                      148 | BIP32_HARD,    // coin_type: Stellar
                      index | BIP32_HARD); // all parameters must be HARDENED

    return privateKey;
}

