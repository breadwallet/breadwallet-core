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
#include "BRArray.h"

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

extern size_t stellarSerializeTransaction(BRStellarAccountID *accountID,
                                       BRStellarFee fee,
                                       BRStellarSequence sequence,
                                       BRStellarTimeBounds *timeBounds,
                                       int numTimeBounds,
                                       BRStellarMemo *memo,
                                       BRStellarOperation * operations,
                                       int numOperations,
                                       uint8_t *signature,
                                       size_t signatureLength,
                                       uint8_t *buffer, size_t bufferSize)
{
    size_t approx_size = 24 + 4 + 8 + (4 + numTimeBounds*16); // First 4 fields
    approx_size += (4 + 4 + 4); // Memo type, time bounds array size, numOperations
    if (numTimeBounds) {
        approx_size += numTimeBounds * 16;
    }
    if (memo) {
        approx_size += 32; // The largest value in a memo object
    }
    if (numOperations) {
        approx_size += numOperations * 256; // Have to figure this out
    }
    if (signature) {
        approx_size += 4 + signatureLength + 3;
    }

    uint8_t *pStart = buffer;
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

    // self.pack_int(data.ext.v)

    return (pCurrent - pStart);
}
