//
//  BRStellarSerialize.c
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "BRStellar.h"
#include "BRStellarBase.h"
#include "BRStellarPrivateStructs.h"
#include "support/BRInt.h"
#include "support/BRCrypto.h"
#include "support/BRKey.h"
#include "BRStellarAccount.h"
#include "ed25519/ed25519.h"

bool validAccountID(BRStellarAccountID * account)
{
    for(int i = 0; i < 32; i++) {
        // If we find any bytes that are not 0 we have something
        if (account->accountID[i]) return true;
    }
    return false;
}

uint8_t * pack_int(uint32_t value, uint8_t * buffer)
{
    buffer[0] = value >> 24;
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = value & 0xff;
    return buffer + 4;
}

uint8_t * pack_uint64(uint64_t i, uint8_t* buffer)
{
    buffer[0] = (i >> 56);
    buffer[1] = (i >> 48) & 0xff;
    buffer[2] = (i >> 40) & 0xff;
    buffer[3] = (i >> 32) & 0xff;
    buffer[4] = (i >> 24) & 0xff;
    buffer[5] = (i >> 16) & 0xff;
    buffer[6] = (i >> 8) & 0xff;
    buffer[7] = (i & 0xff);
    return buffer + 8;
}

uint8_t * pack_fopaque(uint8_t *data, int dataSize, uint8_t *buffer)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    int paddedSize = ((dataSize+3)/4) * 4;
    int padding = paddedSize - dataSize;
    memcpy(buffer, data, dataSize);
    return (buffer + dataSize + padding);
}

uint8_t * pack_opaque(uint8_t *data, int dataSize, uint8_t *buffer)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    buffer = pack_int(dataSize, buffer);
    return pack_fopaque(data, dataSize, buffer);
}

uint8_t * pack_string(const char* data, uint8_t *buffer)
{
    int length = (int)strlen(data);
    buffer = pack_int(length, buffer);
    return pack_fopaque((uint8_t*)data, length, buffer);
}

uint8_t * pack_AccountID(BRStellarAccountID * accountID, uint8_t *buffer)
{
    buffer = pack_int(accountID->accountType, buffer); // Always 0 for ed25519 public key
    return pack_fopaque(accountID->accountID, sizeof(accountID->accountID), buffer);
}

uint8_t * pack_TimeBounds(BRStellarTimeBounds *timeBounds, int numTimeBounds, uint8_t *buffer)
{
    // TimeBounds is an array - so first write out the number of elements
    if (!timeBounds) {
        return pack_int(0, buffer); // array size is zero
    }
    buffer = pack_int(numTimeBounds, buffer);
    for(int i = 0; i < numTimeBounds; i++) {
        // TimeBounds is make up of 2 TimePoint objects (uint64_t)
        buffer = pack_uint64(timeBounds[i].minTime, buffer);
        buffer = pack_uint64(timeBounds[i].maxTime, buffer);
    }
    return buffer;
}

uint8_t * pack_Memo(BRStellarMemo *memo, uint8_t *buffer)
{
    if (!memo) {
        return pack_int(0, buffer); // no memo
    }
    buffer = pack_int(memo->memoType, buffer);
    switch (memo->memoType) {
        case MEMO_NONE:
            break;
        case MEMO_TEXT:
            buffer = pack_string(memo->text, buffer);
            break;
        case MEMO_ID:
            break;
        case MEMO_HASH:
            break;
        case MEMO_RETURN:
            break;
    }
    return buffer;
}

uint8_t * pack_Asset(BRStellarAsset *asset, uint8_t *buffer)
{
    buffer = pack_int(asset->type, buffer);
    switch(asset->type) {
        case ASSET_TYPE_NATIVE:
            // Do nothing
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            buffer = pack_fopaque((uint8_t*)asset->assetCode, 4, buffer);
            buffer = pack_AccountID(&asset->issuer, buffer);
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            buffer = pack_fopaque((uint8_t*)asset->assetCode, 12, buffer);
            buffer = pack_AccountID(&asset->issuer, buffer);
            break;
    }
    return buffer;
}

uint8_t * pack_Op(BRStellarOperation *op, uint8_t *buffer)
{
    // SourceID - for some strange reason the sourceID (optional) is packed as
    // an array. So at a minimum we need to pack the array size (0 or 1).
    if (validAccountID(&op->source)) {
        buffer = pack_int(1, buffer);
        buffer = pack_AccountID(&op->source, buffer);
    } else {
        buffer = pack_int(0, buffer);
    }

    buffer = pack_int(op->type, buffer); // Operation type
    buffer = pack_AccountID(&op->operation.payment.destination, buffer); // DestinationID
    buffer = pack_Asset(&op->operation.payment.asset, buffer); // Asset

    // Amount See `Stellar's documentation on Asset Precision
    // <https://www.stellar.org/developers/guides/concepts/assets.html#amount-precision-and-representation>`_
    // for more information.
    uint64_t amount = (uint64_t)(op->operation.payment.amount * (double)10000000);
    buffer = pack_uint64(amount, buffer);
    return buffer;
}

uint8_t * pack_Operations(BRStellarOperation *operations, int numOperations, uint8_t *buffer)
{
    // Operations is an array - so first write out the number of elements
    if (!operations) {
        return pack_int(0, buffer); // array size is zero
    }
    buffer = pack_int(numOperations, buffer);
    for(int i = 0; i < numOperations; i++) {
        buffer = pack_Op(&operations[i], buffer);
    }
    return buffer;
}

uint8_t * pack_Signatures(uint8_t *signatures, int numSignatures, uint8_t *buffer)
{
    buffer = pack_int(numSignatures, buffer);
    for (int i = 0; i < numSignatures; i++)
    {
        buffer = pack_fopaque(&signatures[i * 68], 4, buffer); // Sig hint
        buffer = pack_opaque(&signatures[(i * 68) + 4], 64, buffer); // Sig
    }
    return buffer;
}

extern size_t stellarSerializeTransaction(BRStellarAccountID *accountID,
                                       BRStellarFee fee,
                                       BRStellarSequence sequence,
                                       BRStellarTimeBounds *timeBounds,
                                       int numTimeBounds,
                                       BRStellarMemo *memo,
                                       BRStellarOperation * operations,
                                       int numOperations,
                                       int version,
                                       uint8_t *signatures,
                                       int numSignatures,
                                       uint8_t **buffer)
{
    size_t approx_size = 24 + 4 + 8 + (4 + numTimeBounds*16) + 4 + 32; // First 4 fields + version + buffer
    approx_size += (4 + (numOperations * 128)) + (4 + (memo ? 32 : 0)) +
                   (numSignatures ? (numSignatures * 72) : 4);

    *buffer = calloc(1, approx_size);
    uint8_t *pStart = *buffer;
    uint8_t *pCurrent = pStart;

    // AccountID - PublicKey object
    pCurrent = pack_AccountID(accountID, pCurrent);

    // Fee - uint32
    pCurrent = pack_int(fee, pCurrent);
    
    // Sequence - SequenceNumber object
    pCurrent = pack_uint64(sequence, pCurrent);

    // TimeBounds - TimeBounds (array of TimePoints)
    pCurrent = pack_TimeBounds(timeBounds, numTimeBounds,pCurrent);

    // Memo - Memo object
    pCurrent = pack_Memo(memo, pCurrent);

    // Operations - array of operations
    pCurrent = pack_Operations(operations, numOperations, pCurrent);

    // The Stellar XDR documentation has a stucture (name "v") which
    // says it is reserved for future use.  Currently all the client libraries
    // simpy pack the value 0.
    pCurrent = pack_int(version, pCurrent);

    // Add the signature if applicable
    if (numSignatures) {
        pCurrent = pack_Signatures(signatures, numSignatures, pCurrent);
    }

    return (pCurrent - pStart);
}

BRStellarSignatureRecord stellarTransactionSign(uint8_t * tx_hash, size_t txHashLength,
                                                uint8_t *privateKey, uint8_t *publicKey)
{
    // Create a signature from the incoming bytes
    unsigned char signature[64];
    ed25519_sign(signature, tx_hash, txHashLength, publicKey, privateKey);

    // This is what they call a decorated signature - it includes
    // a 4-byte hint of what public key to use.
    BRStellarSignatureRecord sig;
    memcpy(sig.signature, &publicKey[28], 4); // Last for bytes of public key
    memcpy(&sig.signature[4], signature, 64);
    return sig;
}
