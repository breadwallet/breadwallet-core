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

size_t pack_int(uint32_t value, uint8_t * buffer)
{
    buffer[0] = value >> 24;
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = value & 0xff;
    return 4;
}

int pack_uint64(uint64_t i, uint8_t* buffer)
{
    buffer[0] = (i >> 56);
    buffer[1] = (i >> 48) & 0xff;
    buffer[2] = (i >> 40) & 0xff;
    buffer[3] = (i >> 32) & 0xff;
    buffer[4] = (i >> 24) & 0xff;
    buffer[5] = (i >> 16) & 0xff;
    buffer[6] = (i >> 8) & 0xff;
    buffer[7] = (i & 0xff);
    return 8;
}

size_t pack_fopaque(uint8_t *data, int dataSize, uint8_t *buffer)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    int paddedSize = ((dataSize+3)/4) * 4;
    int padding = paddedSize - dataSize;
    memcpy(buffer, data, dataSize);
    return (dataSize + padding);
}

size_t pack_string(const char* data, uint8_t *buffer)
{
    int length = (int)strlen(data);
    size_t size = pack_int(length, buffer);
    size += pack_fopaque((uint8_t*)data, length, &buffer[size]);
    return size;
}

size_t pack_AccountID(BRStellarAccountID accountID, uint8_t *buffer)
{
    size_t index = pack_int(accountID.accountType, buffer); // Always 0 for ed25519 public key
    return (index + pack_fopaque(accountID.accountID, sizeof(accountID.accountID), &buffer[index]));
}

size_t pack_TimeBounds(BRStellarTimeBounds *timeBounds, int numTimeBounds, uint8_t *buffer)
{
    // TimeBounds is an array - so first write out the number of elements
    if (!timeBounds) {
        return pack_int(0, buffer); // array size is zero
    }
    size_t size = pack_int(numTimeBounds, buffer);
    for(int i = 0; i < numTimeBounds; i++) {
        // TimeBounds is make up of 2 TimePoint objects (uint64_t)
        size += pack_uint64(timeBounds[i].minTime, &buffer[size]);
        size += pack_uint64(timeBounds[i].maxTime, &buffer[size]);
    }
    return size;
}

size_t pack_Memo(BRStellarMemo *memo, uint8_t *buffer)
{
    if (!memo) {
        return pack_int(0, buffer); // no memo
    }
    size_t size = pack_int(memo->memoType, buffer);
    switch (memo->memoType) {
        case MEMO_NONE:
            break;
        case MEMO_TEXT:
            size += pack_string(memo->text, &buffer[size]);
            break;
        case MEMO_ID:
            break;
        case MEMO_HASH:
            break;
        case MEMO_RETURN:
            break;
    }
    return size;
}

size_t pack_Op(BRStellarOperation *op)
{
    size_t size = 0;
    // SourceID
    // TargetID
    // Asset
    // Amount
    return size;
}

size_t pack_Operations(BRStellarOperation *operations, int numOperations, uint8_t *buffer)
{
    // Operations is an array - so first write out the number of elements
    if (!operations) {
        return pack_int(0, buffer); // array size is zero
    }
    size_t size = pack_int(numOperations, buffer);
    for(int i = 0; i < numOperations; i++) {
        size += pack_Op(&operations[i]);
    }
    return size;
}

extern size_t stellarSerializeTransaction(BRStellarAccountID accountID,
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

    size_t size = 0;

    // AccountID - PublicKey object
    size += pack_AccountID(accountID, buffer);

    // Fee - uint32
    size += pack_int(fee, &buffer[size]);
    
    // Sequence - SequenceNumber object
    size += pack_uint64(sequence, &buffer[size]);

    // TimeBounds - TimeBounds (array of TimePoints)
    size += pack_TimeBounds(timeBounds, numTimeBounds, &buffer[size]);

    // Memo - Memo object
    size += pack_Memo(memo, &buffer[size]);

    // Operations - array of operations
    pack_Operations(operations, numOperations, &buffer[size]);

    // self.pack_int(data.ext.v)

    return size;
}
