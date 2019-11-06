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
#include "BRStellarSerialize.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "BRStellar.h"
#include "BRStellarAccountUtils.h"
#include "support/BRInt.h"
#include "support/BRCrypto.h"
#include "support/BRKey.h"
#include "BRStellarAccount.h"

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
    UInt32SetBE(buffer, value);
    return buffer + 4;
}

uint8_t * unpack_int(uint8_t * buffer, int32_t *value)
{
    assert(value);
    // Stellar does send result code as negative numbers. It should work
    // just to unpack it as a unsigned int and it will get converted to
    // a signed int in the assignment below.
    *value = UInt32GetBE(buffer);
    return buffer+4;
}

uint8_t * unpack_uint(uint8_t * buffer, uint32_t *value)
{
    assert(value);
    // TODO -
    *value = UInt32GetBE(buffer);
    return buffer+4;
}

// A bit of a hack but there are several cases where we need to read a
// uint32_t but only if there is a flag indicating it is available
uint8_t * unpack_uint_ifpresent(uint8_t * buffer, uint8_t *isPresent, uint32_t *value)
{
    assert(value);
    uint32_t checkIfPresent = UInt32GetBE(buffer);
    if (1 == checkIfPresent) {
        *isPresent = 0x01;
        *value = UInt32GetBE(buffer);
    }
    return (buffer + 4 + (checkIfPresent == 1 ? 4 : 0));
}

uint8_t * pack_uint64(uint64_t i, uint8_t* buffer)
{
    UInt64SetBE(buffer, i);
    return buffer+8;
}

uint8_t * unpack_bool(uint8_t * buffer, bool *value)
{
    assert(value);
    uint32_t tmp = UInt32GetBE(buffer);
    *value = tmp == 0 ? false : true;
    return buffer+4;
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

uint8_t * unpack_amount(uint8_t* buffer, double* value)
{
    uint64_t amount = UInt64GetBE(buffer);
    *value = (double)amount / (double)10000000;
    return (buffer + 8);
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
    buffer = unpack_amount(buffer, &op->amount);

    return buffer;
}

uint8_t * unpack_PathPayment(uint8_t *buffer, BRStellarPathPaymentOp * op)
{
    buffer = unpack_Asset(buffer, &op->sendAsset); // Send Asset
    buffer = unpack_amount(buffer, &op->sendMax);
    buffer = unpack_AccountID(buffer, &op->destination); // DestinationID
    buffer = unpack_Asset(buffer, &op->destAsset); // Send Asset
    buffer = unpack_amount(buffer, &op->destAmount);
    // Now unpack any paths - first get the array size
    buffer = unpack_uint(buffer, &op->numPaths);
    assert(op->numPaths <= 5); // From Stellar-transaction.x
    for (int i = 0; i < op->numPaths; i++) {
        buffer = unpack_Asset(buffer, &op->path[i]);
    }
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
    buffer = unpack_amount(buffer, &op->amount);
    buffer = unpack_Price(buffer, &op->price);
    buffer = unpack_int64(buffer, &op->offerID);
    return buffer;
}

uint8_t * unpack_ManageBuyOffer(uint8_t *buffer, BRStellarManageBuyOfferOp * op)
{
    buffer = unpack_Asset(buffer, &op->selling); // Asset
    buffer = unpack_Asset(buffer, &op->buying); // Asset
    buffer = unpack_amount(buffer, &op->amount);
    buffer = unpack_Price(buffer, &op->price);
    buffer = unpack_int64(buffer, &op->offerID);
    return buffer;
}

uint8_t * unpack_PassiveSellOffer(uint8_t *buffer, BRStellarPassiveSellOfferOp * op)
{
    buffer = unpack_Asset(buffer, &op->selling); // Asset
    buffer = unpack_Asset(buffer, &op->buying); // Asset
    buffer = unpack_amount(buffer, &op->amount);
    buffer = unpack_Price(buffer, &op->price);
    return buffer;
}

uint8_t * unpack_CreateAccount(uint8_t *buffer, BRStellarCreateAccountOp * op)
{
    buffer = unpack_AccountID(buffer, &op->account); // Asset
    buffer = unpack_amount(buffer, &op->startingBalance);
    return buffer;
}

uint8_t * unpack_Signer(uint8_t * buffer, BRStellarSigner *signer)
{
    buffer = unpack_uint(buffer, &signer->key.keyType);
    buffer = unpack_fopaque(buffer, &signer->key.key[0], 32);
    return unpack_uint(buffer, &signer->weight);
}

uint8_t * unpack_SetOptions(uint8_t *buffer, BRStellarSetOptionsOp * op)
{
    uint32_t isPresent = 0;
    buffer = unpack_uint(buffer, &isPresent); // inflationDest
    if (1 == isPresent) {
        op->settings[0] = 1;
        buffer = unpack_AccountID(buffer, &op->inflationDest);
    }
    buffer = unpack_uint_ifpresent(buffer, &op->settings[1], &op->clearFlags);
    buffer = unpack_uint_ifpresent(buffer, &op->settings[2], &op->setFlags);
    buffer = unpack_uint_ifpresent(buffer, &op->settings[3], &op->masterWeight);
    buffer = unpack_uint_ifpresent(buffer, &op->settings[4], &op->lowThreshold);
    buffer = unpack_uint_ifpresent(buffer, &op->settings[5], &op->medThreshold);
    buffer = unpack_uint_ifpresent(buffer, &op->settings[6], &op->highThreshold);

    buffer = unpack_uint(buffer, &isPresent); // homeDomain
    if (1 == isPresent) {
        op->settings[7] = 1;
        buffer = unpack_string(buffer, &op->homeDomain[0], sizeof(op->homeDomain)/sizeof(char));
    }

    buffer = unpack_uint(buffer, &isPresent); // signer
    if (1 == isPresent) {
        op->settings[8] = 1;
        buffer = unpack_Signer(buffer, &op->signer);
    }
    
    return buffer;
}

uint8_t * unpack_ChangeTrust(uint8_t * buffer, BRStellarChangeTrustOp * op)
{
    buffer = unpack_Asset(buffer, &op->line); // Asset
    return unpack_uint64(buffer, &op->limit);
}

uint8_t * unpack_AllowTrust(uint8_t * buffer, BRStellarAllowTrustOp * op)
{
    buffer = unpack_AccountID(buffer, &op->trustor);
    buffer = unpack_uint(buffer, &op->assetType);
    if (op->assetType == ASSET_TYPE_CREDIT_ALPHANUM4) {
        buffer = unpack_fopaque(buffer, (uint8_t*)&op->assetCode, 4);
    } else if (op->assetType == ASSET_TYPE_CREDIT_ALPHANUM12) {
        buffer = unpack_fopaque(buffer, (uint8_t*)&op->assetCode, 12);
    } else {
        // Log ERROR - return NULL?
    }
    return unpack_bool(buffer, &op->authorize);
}

uint8_t * unpack_AccountMerge(uint8_t * buffer, BRStellarAccountMergeOp * op)
{
    // The only thing an account merge operation has is a destination
    buffer = unpack_AccountID(buffer, &op->destination);
    return buffer;
}

uint8_t * unpack_BumpSequence(uint8_t * buffer, BRStellarBumpSequenceOp * op)
{
    return unpack_uint64(buffer, &op->sequence);
}

uint8_t * unpack_ManageData(uint8_t * buffer, BRStellarManageDataOp * op)
{
    // Key/value pair
    buffer = unpack_string(buffer, &op->key[0], 64);
    // For some (strange) reason the value is packed as an array. BUT there
    // can only be a single item in the array and that item cannot be more
    // than 64 characters. Perhaps someone thought they would support multiple
    // values at some point?
    uint32_t arraySize = 0;
    buffer = unpack_uint(buffer, &arraySize); // Throw this away since always 1
    return unpack_string(buffer, &op->value[0], 64);
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
        case ST_OP_CREATE_ACCOUNT:
            return unpack_CreateAccount(buffer, &op->operation.createAccount);
        case ST_OP_PAYMENT:
            return unpack_Payment(buffer, &op->operation.payment);
        case ST_OP_PATH_PAYMENT:
            return unpack_PathPayment(buffer, &op->operation.pathPayment);
        case ST_OP_MANAGE_SELL_OFFER:
            return unpack_ManageSellOffer(buffer, &op->operation.manageSellOffer);
        case ST_OP_CREATE_PASSIVE_SELL_OFFER:
            return unpack_PassiveSellOffer(buffer, &op->operation.passiveSellOffer);
        case ST_OP_SET_OPTIONS:
            return unpack_SetOptions(buffer, &op->operation.options);
        case ST_OP_CHANGE_TRUST:
            return unpack_ChangeTrust(buffer, &op->operation.changeTrust);
        case ST_OP_ALLOW_TRUST:
            return unpack_AllowTrust(buffer, &op->operation.allowTrust);
        case ST_OP_ACCOUNT_MERGE:
            return unpack_AccountMerge(buffer, &op->operation.accountMerge);
        case ST_OP_BUMP_SEQUENCE:
            return unpack_BumpSequence(buffer, &op->operation.bumpSequence);
        case ST_OP_MANAGE_DATA:
            return unpack_ManageData(buffer, &op->operation.manageData);
        case ST_OP_MANAGE_BUY_OFFER:
            return unpack_ManageBuyOffer(buffer, &op->operation.manageBuyOffer);
        case ST_OP_INFLATION:
            return buffer; // Nothing to do here. The inflation request has no data.
            break;
        default:
            // Log and quit now.
            break;
    }
    return NULL; // Causes the parse to quit - and return an error
}

uint8_t * pack_Operations(BRArrayOf(BRStellarOperation) operations, uint8_t *buffer)
{
    // Operations is an array - so first write out the number of elements
    if (!operations) {
        return pack_int(0, buffer); // array size is zero
    }
    uint32_t numOperations = (uint32_t)array_count(operations);
    buffer = pack_int(numOperations, buffer);
    for(int i = 0; i < numOperations; i++) {
        buffer = pack_Op(&operations[i], buffer);
    }
    return buffer;
}

uint8_t * unpack_Operations(uint8_t *buffer, size_t bufferLength, BRArrayOf(BRStellarOperation) *ops)
{
    // Operations is an array - so first write out the number of elements
    uint32_t numOperations = 0;
    buffer = unpack_uint(buffer, &numOperations);
    if (numOperations > 0) {
        for(int i = 0; i < numOperations; i++) {
            BRStellarOperation op;
            memset(&op, 0x00, sizeof(BRStellarOperation));
            buffer = unpack_Op(buffer, &op);
            array_add(*ops, op);
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

uint8_t * unpack_OfferEntry(uint8_t * buffer, BRStellarOfferEntry * offerEntry)
{
    buffer = unpack_AccountID(buffer, &offerEntry->sellerID);
    buffer = unpack_uint64(buffer, &offerEntry->offerID);
    buffer = unpack_Asset(buffer, &offerEntry->selling);
    buffer = unpack_Asset(buffer, &offerEntry->buying);
    buffer = unpack_amount(buffer, &offerEntry->amount);
    buffer = unpack_Price(buffer, &offerEntry->price);
    buffer = unpack_uint(buffer, &offerEntry->flags);
    int32_t v;
    return unpack_int(buffer, &v);
}

uint8_t * unpack_ClaimedOffer(uint8_t * buffer, BRStellarClaimOfferAtom * offerAtom)
{
    buffer = unpack_AccountID(buffer, &offerAtom->sellerID);
    buffer = unpack_uint64(buffer, &offerAtom->offerID);
    buffer = unpack_Asset(buffer, &offerAtom->assetSold);
    buffer = unpack_amount(buffer, &offerAtom->amountSold);
    buffer = unpack_Asset(buffer, &offerAtom->assetBought);
    return unpack_amount(buffer, &offerAtom->amountBought);
}

uint8_t * unpack_ManageOfferSuccessResult(uint8_t * buffer, BRStellarManageOfferResult * offerResult)
{
    uint32_t numClaimedOffers = 0;
    buffer = unpack_uint(buffer, &numClaimedOffers);
    if (numClaimedOffers > 0) {
        // Delete the existing array if there is one
        if (offerResult->claimOfferAtom) {
            array_free(offerResult->claimOfferAtom);
        }
        array_new(offerResult->claimOfferAtom, numClaimedOffers);
        for (int i = 0; i < numClaimedOffers; i++) {
            BRStellarClaimOfferAtom offerAtom;
            buffer = unpack_ClaimedOffer(buffer, &offerAtom);
            array_add(offerResult->claimOfferAtom, offerAtom);
        }
    }
    buffer = unpack_uint(buffer, &offerResult->offerType);
    if (offerResult->offerType == ST_MANAGE_OFFER_CREATED ||
        offerResult->offerType == ST_MANAGE_OFFER_UPDATED ||
        offerResult->offerType == ST_MANAGE_OFFER_DELETED) {
        buffer = unpack_OfferEntry(buffer, &offerResult->offer);
    }
    return buffer;
}

uint8_t * unpack_InflationResult(uint8_t * buffer, BRStellarInflationOp *op)
{
    // Get the number of inflation payouts
    uint32_t numPayouts = 0;
    buffer = unpack_uint(buffer, &numPayouts);
    if (numPayouts > 0) {
        if (op->payouts) {
            array_free(op->payouts);
        }
        array_new(op->payouts, numPayouts);
        for (int i = 0; i < numPayouts; i++) {
            BRStellarInflationPayout payout;
            buffer = unpack_AccountID(buffer, &payout.destination);
            buffer = unpack_amount(buffer, &payout.amount);
            array_add(op->payouts, payout);
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
                                       BRArrayOf(BRStellarOperation) operations,
                                       uint32_t version,
                                       uint8_t *signatures,
                                       int numSignatures,
                                       uint8_t **buffer)
{
    size_t approx_size = 24 + 4 + 8 + (4 + numTimeBounds*16) + 4 + 32; // First 4 fields + version + buffer
    approx_size += (4 + (array_count(operations) * 128)) + (4 + (memo ? 32 : 0)) +
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
    pCurrent = pack_Operations(operations, pCurrent);

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

bool stellarDeserializeTransaction(BRStellarAccountID *accountID,
                                BRStellarFee *fee,
                                BRStellarSequence *sequence,
                                BRStellarTimeBounds **timeBounds,
                                uint32_t *numTimeBounds,
                                BRStellarMemo **memo,
                                BRArrayOf(BRStellarOperation) *ops,
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
    pCurrent = unpack_Operations(pCurrent, pEnd - pCurrent, ops);
    
    // The Stellar XDR documentation has a stucture (name "v") which
    // says it is reserved for future use.  Currently all the client libraries
    // simpy pack the value 0.
    pCurrent = unpack_int(pCurrent, version);

    // If we still have more to unpack then we must have a signature.
    // We need at least 4 more bytes to read the array length.
    if (pEnd - pCurrent > 4) {
        pCurrent = unpack_Signatures(pCurrent, signature, signatureLength);
    }
    
    return true;
}

int stellarDeserializeResultXDR(uint8_t * result_xdr, size_t result_length, BRArrayOf(BRStellarOperation) *ops,
                                 BRStellarTransactionResult * txResult)
{
    // The result_xdr has the following content
    // 1. Fee (8 bytes for some reason, instead of 4)
    // 2. Result code for the transaction (4 bytes)
    // 3. An array of operation result codes
    // 4. The version (4 bytes)
    uint8_t * pCurrent = result_xdr;
    pCurrent = unpack_uint64(pCurrent, &txResult->fee);
    pCurrent = unpack_int(pCurrent, &txResult->resultCode);
    if (txResult->resultCode == ST_TX_SUCCESS || txResult->resultCode == ST_TX_FAILED)
    {
        // Get the operation results
        uint32_t numOperations;
        pCurrent = unpack_uint(pCurrent, &numOperations);
        // Make sure that our operation array is large enough. It is possible that
        // this function gets called without ever parsing out the transaction info
        if (numOperations > array_count(ops)) {
            array_set_count(*ops, numOperations);
        }
        for(int i = 0; i < numOperations; i++) {
            int32_t opInner = 0;
            BRStellarOperation *op = &((*ops)[i]);
            pCurrent = unpack_int(pCurrent, &opInner);
            if (opInner == ST_OP_RESULT_CODE_INNER) {
                // Get the operation type and result code for this operation
                pCurrent = unpack_uint(pCurrent, &op->type);
                pCurrent = unpack_int(pCurrent, &op->resultCode);
                switch (op->type) {
                    case ST_OP_ACCOUNT_MERGE:
                        if (0 == op->resultCode) { // 0 is alwasy success
                            pCurrent = unpack_amount(pCurrent, &op->operation.accountMerge.balance);
                        }
                        break;
                    case ST_OP_MANAGE_SELL_OFFER:
                        if (0 == op->resultCode) {
                            pCurrent = unpack_ManageOfferSuccessResult(pCurrent, &op->operation.manageSellOffer.offerResult);
                        }
                        break;
                    case ST_OP_MANAGE_BUY_OFFER:
                        if (0 == op->resultCode) {
                            pCurrent = unpack_ManageOfferSuccessResult(pCurrent, &op->operation.manageBuyOffer.offerResult);
                        }
                        break;
                    case ST_OP_CREATE_PASSIVE_SELL_OFFER:
                        if (0 == op->resultCode) {
                            pCurrent = unpack_ManageOfferSuccessResult(pCurrent, &op->operation.passiveSellOffer.offerResult);
                        }
                        break;
                    case ST_OP_INFLATION:
                        if (0 == op->resultCode) {
                            pCurrent = unpack_InflationResult(pCurrent, &op->operation.inflation);
                        }
                        break;
                    case ST_OP_CREATE_ACCOUNT:
                    case ST_OP_PAYMENT:
                    case ST_OP_SET_OPTIONS:
                    case ST_OP_CHANGE_TRUST:
                    case ST_OP_ALLOW_TRUST:
                    case ST_OP_MANAGE_DATA:
                    case ST_OP_BUMP_SEQUENCE:
                        // Nothing more to do here - no data for these types, just a result code.
                        break;
                    case ST_OP_PATH_PAYMENT: // Need to find a real one of these for testing
                    default:
                        return ST_XDR_UNSUPPORTED_OPERATION;
                }
            }
        }
        uint32_t version = 0;
        pCurrent = unpack_uint(pCurrent, &version);
    } else {
        return ST_XDR_FAILURE;
    }
    
    return ST_XDR_SUCCESS;
}
