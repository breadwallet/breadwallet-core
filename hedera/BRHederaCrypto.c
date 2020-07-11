/**
*/

#include "BRHederaCrypto.h"
#include "support/BRBIP32Sequence.h"
#include "vendor/ed25519/ed25519.h"

#include <stdlib.h>
#include <assert.h>

// Forward declarations
static BRKey deriveHederaKeyFromSeed (UInt512 seed, uint32_t index);

void hederaKeyGetPublicKey (BRKey key, uint8_t * publicKey)
{
    unsigned char privateKey[64] = {0};
    ed25519_create_keypair(publicKey, privateKey, key.secret.u8);
    memset(privateKey, 0x00, 64);
}

BRKey hederaKeyCreate (UInt512 seed)
{
    return deriveHederaKeyFromSeed(seed, 0);
}


#define ED25519_SEED_KEY "ed25519 seed"
static void _deriveKeyImpl(BRKey *key, const void *seed, size_t seedLen, int depth, va_list vlist)
{
    UInt512 I;
    UInt256 secret, chainCode;

    assert(key != NULL);
    assert(seed != NULL || seedLen == 0);

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
            //assert(seq & 0xF0000000); // Needs to be hardened
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

static void _hederaDeriveKey(BRKey *key, const void *seed, size_t seedLen, int depth, ...)
{
    va_list ap;

    va_start(ap, depth);
    _deriveKeyImpl(key, seed, seedLen, depth, ap);
    va_end(ap);
}

static BRKey deriveHederaKeyFromSeed (UInt512 seed, uint32_t index)
{
    BRKey privateKey;

    // The BIP32 privateKey for m/44'/148'/%d' where %d is the key index
    // The stellar system only supports ed25519 key pairs. There are some restrictions when it comes
    // to creating key pairs for ed25519, described here:
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0005.md
    _hederaDeriveKey(&privateKey, &seed, sizeof(UInt512), 4,
                     44 | BIP32_HARD,     // purpose  : BIP-44
                     3030 | BIP32_HARD,    // coin_type: HBAR
                     0 | BIP32_HARD,
                     0 | BIP32_HARD);

    return privateKey;
}



