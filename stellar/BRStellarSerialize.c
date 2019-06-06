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
#include "BRStellarAccountUtils.h"
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

uint8_t * pack_int(int32_t value, uint8_t * buffer)
{
    // The XDRLIB code just packs a signed integer as unsigned
    UInt32SetBE(buffer, value);
    return buffer + 4;
}

uint8_t * unpack_uint(uint8_t * buffer, uint32_t *value)
{
    assert(value);
    *value = UInt32GetBE(buffer);
    return buffer+4;
}

uint8_t * unpack_int(uint8_t * buffer, int32_t *value)
{
    assert(value);
    // TODO - check if there is any difference in the signed/unsigned 32-bit integers
    // with respect to the XDR library
    *value = UInt32GetBE(buffer);
    return buffer+4;
}

uint8_t * pack_uint64(uint64_t i, uint8_t* buffer)
{
    UInt64SetBE(buffer, i);
    return buffer+8;
}

// 64-bit unsigned integer
uint8_t * unpack_uint64(uint8_t * buffer, uint64_t * value)
{
    *value = UInt64GetBE(buffer);
    return buffer+8;
    // NOTE - the Python xdrlib.py does it like this...
    // uint32_t high;
    // uint32_t low;
    // buffer = unpack_uint(buffer, &high);
    // buffer = unpack_uint(buffer, &low);
    // *value = (uint64_t)high << 32 | low;
}

// 64-bit signed integer
uint8_t * unpack_int64(uint8_t * buffer, int64_t * value)
{
    uint64_t tmpValue = UInt64GetBE(buffer);
    *value = tmpValue;
    return buffer+8;
    // TODO - figure out what this code does - if a signed 64-bit
    // integer has the sign bit set then this magic happens
    // The Python xdrlib.py unpack a signed 64-bit integer like this
    // x = self.unpack_uhyper()
    // if x >= 0x8000000000000000:
    //    x = x - 0x10000000000000000
    //    return x
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

uint8_t * unpack_fopaque(uint8_t *buffer, uint8_t *data, size_t dataSize)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    size_t paddedSize = ((dataSize+3)/4) * 4;
    // NOTE *** we assume that the caller knows the input buffer is
    // big enough but if not then it will just copy crap into "data"
    if (dataSize > 0) {
        memcpy(data, buffer, dataSize);
    }
    return (buffer + paddedSize);
}

uint8_t * pack_opaque(uint8_t *data, int dataSize, uint8_t *buffer)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    buffer = pack_int(dataSize, buffer);
    return pack_fopaque(data, dataSize, buffer);
}

uint8_t * unpack_opaque(uint8_t *buffer, uint8_t *data, uint32_t dataSize)
{
    // See if the datasize is a multiple of 4 - and determine how many
    // bytes we need to pad at the end of this opaque data
    uint32_t expectedDataSize = 0;
    buffer = unpack_uint(buffer, &expectedDataSize);
    assert(expectedDataSize == dataSize);
    return unpack_fopaque(buffer, data, dataSize);
}

uint8_t * pack_string(const char* data, uint8_t *buffer)
{
    int length = (int)strlen(data);
    buffer = pack_int(length, buffer);
    return pack_fopaque((uint8_t*)data, length, buffer);
}

uint8_t * unpack_string(uint8_t *buffer, char* data, size_t dataLength)
{
    uint32_t length = 0;
    buffer = unpack_uint(buffer, &length);
    assert(length <= dataLength);
    return unpack_fopaque(buffer, (uint8_t*)data, length);
}

uint8_t * pack_AccountID(BRStellarAccountID * accountID, uint8_t *buffer)
{
    buffer = pack_int(accountID->accountType, buffer); // Always 0 for ed25519 public key
    return pack_fopaque(accountID->accountID, sizeof(accountID->accountID), buffer);
}

uint8_t * unpack_AccountID(uint8_t *buffer, BRStellarAccountID * accountID)
{
    buffer = unpack_uint(buffer, &accountID->accountType); // Always 0 for ed25519 public key
    return unpack_fopaque(buffer, &accountID->accountID[0], sizeof(accountID->accountID));
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

uint8_t * unpack_TimeBounds(uint8_t *buffer, size_t bufferLength, BRStellarTimeBounds **timeBounds, uint32_t *numTimeBounds)
{
    // TimeBounds is an array - so first get the number of items
    buffer = unpack_uint(buffer, numTimeBounds);
    assert(bufferLength > (*numTimeBounds * sizeof(BRStellarTimeBounds)));
    if (*numTimeBounds > 0) {
        // Allocate the required amount of memory
        *timeBounds = calloc(*numTimeBounds, sizeof(BRStellarTimeBounds));
        for(int i = 0; i < *numTimeBounds; i++) {
            // TimeBounds is make up of 2 TimePoint objects (uint64_t)
            buffer = unpack_uint64(buffer, &(*timeBounds)[i].minTime);
            buffer = unpack_uint64(buffer, &(*timeBounds)[i].maxTime);
        }
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

uint8_t * unpack_Memo(uint8_t *buffer, size_t bufferLength, BRStellarMemo **memo)
{
    uint32_t memoType = 0;
    buffer = unpack_uint(buffer, &memoType);
    if (memoType == MEMO_NONE) {
        return buffer; // No memo
    }
    assert(bufferLength > sizeof(BRStellarMemo));
    *memo = calloc(1, sizeof(BRStellarMemo));
    switch (memoType) {
        case MEMO_NONE:
            break;
        case MEMO_TEXT:
            buffer = unpack_string(buffer, (*memo)->text, sizeof((*memo)->text));
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

uint8_t * unpack_Asset(uint8_t *buffer, BRStellarAsset *asset)
{
    buffer = unpack_uint(buffer, &asset->type);
    switch(asset->type) {
        case ASSET_TYPE_NATIVE:
            // Do nothing
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            buffer = unpack_fopaque(buffer, (uint8_t*)&asset->assetCode, 4);
            buffer = unpack_AccountID(buffer, &asset->issuer);
            // for testing - turn this accountID into a stellar string
            BRKey key;
            memcpy(&key.pubKey[0], &asset->issuer.accountID[0], 32);
            BRStellarAddress address = createStellarAddressFromPublicKey(&key);
            printf("Stellar Address: %s\n", address.bytes);
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            buffer = unpack_fopaque(buffer, (uint8_t*)&asset->assetCode, 12);
            buffer = unpack_AccountID(buffer, &asset->issuer);
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

uint8_t * unpack_Payment(uint8_t *buffer, BRStellarPaymentOp * op)
{
    buffer = unpack_AccountID(buffer, &op->destination); // DestinationID
    buffer = unpack_Asset(buffer, &op->asset); // Asset
    
    // Amount See `Stellar's documentation on Asset Precision
    // <https://www.stellar.org/developers/guides/concepts/assets.html#amount-precision-and-representation>`_
    // for more information.
    uint64_t amount = 0;
    printf("\n");
    buffer = unpack_uint64(buffer, &amount);
    assert(amount < 0x8000000000000000); // 9,223,372,036,854,775,807, largest amount allowed
    // Amounts are scaled up by 10,000,000 during transport, we need to divide by the same
    // value to get the correct amount number.
    op->amount = (double)amount / (double)10000000;
    printf("Amount: %f\n", op->amount);

    return buffer;
}

uint8_t * unpack_Price(uint8_t *buffer, BRStellarPrice *price)
{
    buffer = unpack_int(buffer, &price->n);
    return unpack_int(buffer, &price->d);
}

uint8_t * unpack_ManageSellOffer(uint8_t *buffer, BRStellarManageSellOfferOp * op)
{
    buffer = unpack_Asset(buffer, &op->selling); // Asset
    buffer = unpack_Asset(buffer, &op->buying); // Asset
    buffer = unpack_int64(buffer, &op->amount);
    buffer = unpack_Price(buffer, &op->price);
    buffer = unpack_int64(buffer, &op->offerID);
    return buffer;
}

uint8_t * unpack_Op(uint8_t *buffer, BRStellarOperation *op)
{
    // See if the optional account ID is there
    uint32_t arrayLength = 0;
    buffer = unpack_uint(buffer, &arrayLength);
    if (arrayLength > 0) {
        assert(arrayLength == 1);
        buffer = unpack_AccountID(buffer, &op->source);
    }
    buffer = unpack_uint(buffer, &op->type); // Operation type
    switch(op->type) {
        case PAYMENT:
            return unpack_Payment(buffer, &op->operation.payment);
        case MANAGE_SELL_OFFER:
            return unpack_ManageSellOffer(buffer, &op->operation.mangeSellOffer);
        case CREATE_ACCOUNT:
        case PATH_PAYMENT:
        case CREATE_PASSIVE_SELL_OFFER:
        case SET_OPTIONS:
        case CHANGE_TRUST:
        case ALLOW_TRUST:
        case ACCOUNT_MERGE:
        case INFLATION:
        case MANAGE_DATA:
        case BUMP_SEQUENCE:
        case MANAGE_BUY_OFFER:
            break;
        default:
            // Log and quit now.
            break;
    }
    return NULL; // Causes the parse to quit - and return an error
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

uint8_t * unpack_Operations(uint8_t *buffer, size_t bufferLength,
                            BRStellarOperation **operations, uint32_t *numOperations)
{
    // Operations is an array - so first write out the number of elements
    buffer = unpack_uint(buffer, numOperations);
    if (*numOperations > 0) {
        *operations = calloc(*numOperations, sizeof(BRStellarOperation));
        for(int i = 0; i < *numOperations; i++) {
            buffer = unpack_Op(buffer, &(*operations)[i]);
        }
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

uint8_t * unpack_Signatures(uint8_t *buffer, uint8_t **signatures, uint32_t *numSignatures)
{
    buffer = unpack_uint(buffer, numSignatures);
    if (*numSignatures > 0) {
        *signatures = calloc(*numSignatures, 68);
        for (int i = 0; i < *numSignatures; i++)
        {
            buffer = unpack_fopaque(buffer, &(*signatures)[i * 68], 4); // Sig hint
            buffer = unpack_opaque(buffer, &(*signatures)[(i * 68) + 4], 64); // Sig
        }
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
                                       uint32_t version,
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

bool stellarDeserializeTransaction(BRStellarAccountID *accountID,
                                BRStellarFee *fee,
                                BRStellarSequence *sequence,
                                BRStellarTimeBounds **timeBounds,
                                uint32_t *numTimeBounds,
                                BRStellarMemo **memo,
                                BRStellarOperation **operations,
                                uint32_t *numOperations,
                                int32_t *version,
                                uint8_t **signature,
                                uint32_t *signatureLength,
                                uint8_t *buffer, size_t bufferLength)
{
    uint8_t *pCurrent = buffer;
    uint8_t *pEnd = buffer + bufferLength;

    // A lot of asserts
    assert(buffer);
    assert(bufferLength > 30);

    assert((pEnd - pCurrent) > sizeof(BRStellarAccountID));
    pCurrent = unpack_AccountID(pCurrent, accountID);
    assert(pCurrent - buffer == 36);
    // Fee - uint32
    assert((pEnd - pCurrent) > sizeof(BRStellarFee));
    pCurrent = unpack_uint(pCurrent, fee);
    
    // Sequence - SequenceNumber object
    assert((pEnd - pCurrent) > sizeof(BRStellarSequence));
    pCurrent = unpack_int64(pCurrent, sequence);
    
    // TimeBounds - TimeBounds (array of TimePoints)
    assert((pEnd - pCurrent) > sizeof(uint32_t)); // Can we read the array length
    pCurrent = unpack_TimeBounds(pCurrent, pEnd - pCurrent, timeBounds, numTimeBounds);
    
    // Memo - Memo object
    assert((pEnd - pCurrent) > sizeof(uint32_t)); // Can we read the memo length
    pCurrent = unpack_Memo(pCurrent, pEnd - pCurrent, memo);
    
    // Operations - array of operations
    assert((pEnd - pCurrent) > sizeof(uint32_t)); // Can we read the array length
    pCurrent = unpack_Operations(pCurrent, pEnd - pCurrent, operations, numOperations);
    
    // The Stellar XDR documentation has a stucture (name "v") which
    // says it is reserved for future use.  Currently all the client libraries
    // simpy pack the value 0.
    pCurrent = unpack_int(pCurrent, version);

    // If we still have more to unpack then we must have a signature
    if (pEnd - pCurrent > 4) {
        pCurrent = unpack_Signatures(pCurrent, signature, signatureLength);
    }
    
    return true;
}
