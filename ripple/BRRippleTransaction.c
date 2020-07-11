//
//  BRRippleTransaction.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BRRipple.h"
#include "BRRippleBase.h"
#include "BRRipplePrivateStructs.h"
#include "BRRippleSerialize.h"
#include "BRRippleSignature.h"
#include "BRRippleAccount.h"
#include "BRCrypto.h"
#include "BRArray.h"
#include "BRInt.h"

static VLBytes *
rippleVLBytesClone (VLBytes *bytes) {
    if (NULL == bytes) return NULL;

    VLBytes *clone = createVLBytes (bytes->length);
    clone->length = bytes->length;
    memcpy (clone->value, bytes->value, clone->length);

    return clone;
}

static BRRippleMemo
rippleMemoClone (BRRippleMemo memo) {
    return (BRRippleMemo) {
        rippleVLBytesClone (memo.memoType),
        rippleVLBytesClone (memo.memoData),
        rippleVLBytesClone (memo.memoFormat)
    };
}

static BRRippleMemoNode *
rippleMemoNodeClone (BRRippleMemoNode *node) {
    if (NULL == node) return NULL;

    BRRippleMemoNode *clone = calloc (1, sizeof (BRRippleMemoNode));
    clone->memo = rippleMemoClone (node->memo);
    clone->next = rippleMemoNodeClone (node->next);

    return NULL;
}

typedef struct _txPaymentRecord {
    // The address to whom the payment is being sent
    BRRippleAddress targetAddress;

    // The payment amount (currently only supporting XRP drops
    BRRippleAmount amount;

    // (Optional) Arbitrary tag that identifies the reason for the payment
    // to the destination, or a hosted recipient to pay.
    BRRippleDestinationTag destinationTag;

    // (Optional) Arbitrary 256-bit hash representing a specific
    // reason or identifier for this payment.
    uint8_t invoiceId[32];

    // (Optional) Highest amount of source currency this transaction is
    // allowed to cost, including transfer fees, exchange rates, and slippage.
    // Does not include the XRP destroyed as a cost for submitting the transaction.
    // For non-XRP amounts, the nested field names MUST be lower-case.
    // Must be supplied for cross-currency/cross-issue payments.
    // Must be omitted for XRP-to-XRP payments.
    BRRippleAmount sendMax;

    // (Optional) Minimum amount of destination currency this transaction should deliver.
    // Only valid if this is a partial payment.
    //For non-XRP amounts, the nested field names are lower-case.
    BRRippleAmount deliverMin;

} BRRipplePaymentTxRecord;

struct BRRippleSerializedTransactionRecord {
    uint32_t size;
    uint8_t  *buffer;
    uint8_t  txHash[32];
};
typedef struct BRRippleSerializedTransactionRecord *BRRippleSerializedTransaction;

extern BRRippleSerializedTransaction
rippleSerializedTransactionClone (const BRRippleSerializedTransaction transaction) {
    BRRippleSerializedTransaction clone = calloc (1, sizeof (struct BRRippleSerializedTransactionRecord));
    memcpy (clone, transaction, sizeof (struct BRRippleSerializedTransactionRecord));

    clone->buffer = malloc (clone->size);
    memcpy (clone->buffer, transaction->buffer, clone->size);
    return clone;
}

struct BRRippleTransactionRecord {
    
    // COMMON FIELDS

    // The address of the account "doing" the transaction
    BRRippleAddress sourceAddress;

    // The Transaction type
    BRRippleTransactionType transactionType;

    // The base fee according to the Ripple network.
    BRRippleFeeBasis feeBasis;

    BRRippleUnitDrops fee; // The actual fee sent in the tx
    
    // The next valid sequence number for the initiating account
    BRRippleSequence sequence;

    BRRippleFlags flags;
    BRRippleLastLedgerSequence lastLedgerSequence;

    // The account
    BRKey publicKey;

    // The ripple payment information
    // TODO in the future if more transaction are supported this could
    // be changed to a union of the various types
    BRRipplePaymentTxRecord payment;

    BRRippleSerializedTransaction signedBytes;

    BRRippleSignatureRecord signature;

    // Other fields that might show up when deserializing

    // Hash value identifying another transaction. If provided, this transaction
    //is only valid if the sending account's previously-sent transaction matches the provided hash.
    BRRippleTransactionHash accountTxnID;

    // Arbitrary integer used to identify the reason for this payment,
    // or a sender on whose behalf this transaction is made. Conventionally,
    // a refund should specify the initial payment's SourceTag as the refund
    // payment's DestinationTag.
    BRRippleSourceTag sourceTag;

    BRRippleMemoNode * memos;
};

void rippleSerializedTransactionRecordFree(BRRippleSerializedTransaction * signedBytes)
{
    assert(signedBytes);
    assert(*signedBytes);
    if ((*signedBytes)->buffer) {
        free((*signedBytes)->buffer);
    }
    free(*signedBytes);
}

static BRRippleTransaction createTransactionObject()
{
    BRRippleTransaction transaction = calloc (1, sizeof (struct BRRippleTransactionRecord));
    assert(transaction);
    return transaction;
}

extern BRRippleTransaction
rippleTransactionCreate(BRRippleAddress sourceAddress,
                        BRRippleAddress targetAddress,
                        BRRippleUnitDrops amount, // For now assume XRP drops.
                        BRRippleFeeBasis feeBasis)
{
    BRRippleTransaction transaction = createTransactionObject();

    // Common fields
    transaction->feeBasis = feeBasis;
    transaction->fee = 0; // Don't know it yet
    transaction->sourceAddress = rippleAddressClone (sourceAddress);
    transaction->transactionType = RIPPLE_TX_TYPE_PAYMENT;
    transaction->flags = 0x80000000; // tfFullyCanonicalSig
    transaction->lastLedgerSequence = 0;

    // Payment information
    transaction->payment.targetAddress = rippleAddressClone (targetAddress);
    transaction->payment.amount.currencyType = 0; // XRP
    transaction->payment.amount.amount.u64Amount = amount; // XRP only
    
    transaction->signedBytes = NULL;

    return transaction;
}

extern BRRippleTransaction
rippleTransactionClone (BRRippleTransaction transaction) {
    BRRippleTransaction clone = createTransactionObject();
    memcpy (clone, transaction, sizeof(struct BRRippleTransactionRecord));

    if (transaction->payment.targetAddress)
        clone->payment.targetAddress = rippleAddressClone (transaction->payment.targetAddress);

    if (transaction->sourceAddress)
        clone->sourceAddress = rippleAddressClone (transaction->sourceAddress);

    if (transaction->signedBytes)
        clone->signedBytes = rippleSerializedTransactionClone (transaction->signedBytes);

    if (transaction->memos)
        clone->memos = rippleMemoNodeClone (transaction->memos);

    return clone;
}

extern void rippleTransactionFree(BRRippleTransaction transaction)
{
    assert(transaction);

    if (transaction->payment.targetAddress) {
        rippleAddressFree(transaction->payment.targetAddress);
    }
    if (transaction->sourceAddress) {
        rippleAddressFree(transaction->sourceAddress);
    }

    if (transaction->signedBytes) {
        rippleSerializedTransactionRecordFree(&transaction->signedBytes);
        transaction->signedBytes = NULL;
    }
    if (transaction->memos) {
        memoListFree(transaction->memos);
    }
    free(transaction);
}

int setFieldInfo(BRRippleField *fields, BRRippleTransaction transaction,
                  uint8_t * signature, int sig_length)
{
    int index = 0;
    
    // Convert all the content to ripple fields
    fields[index].typeCode = 8;
    fields[index].fieldCode = 1;
    fields[index++].data.address = transaction->sourceAddress;

    fields[index].typeCode = 1;
    fields[index].fieldCode = 2;
    fields[index++].data.i16 = transaction->transactionType;

    fields[index].typeCode = 2;
    fields[index].fieldCode = 4;
    fields[index++].data.i32 = transaction->sequence;

    fields[index].typeCode = 6;
    fields[index].fieldCode = 8;
    fields[index++].data.i64 = transaction->fee;
    
    // Payment info
    fields[index].typeCode = 8;
    fields[index].fieldCode = 3;
    fields[index++].data.address = transaction->payment.targetAddress;

    fields[index].typeCode = 6;
    fields[index].fieldCode = 1;
    fields[index++].data.i64 = transaction->payment.amount.amount.u64Amount; // XRP only

    // Public key info
    fields[index].typeCode = 7;
    fields[index].fieldCode = 3;
    fields[index++].data.publicKey = transaction->publicKey;

    fields[index].typeCode = 2;
    fields[index].fieldCode = 2;
    fields[index++].data.i32 = transaction->flags;

    // Always add in the destination tag - the Ripple system will ignore it
    // but exchanges use it to identify internally hosted accounts that share
    // a single ripple address.
    fields[index].typeCode = 2;
    fields[index].fieldCode = 14;
    fields[index++].data.i32 = transaction->payment.destinationTag;

    if (signature) {
        fields[index].typeCode = 7;
        fields[index].fieldCode = 4;
        memcpy(&fields[index].data.signature.signature, signature, sig_length);
        fields[index++].data.signature.sig_length = sig_length;
    }
    
    if (transaction->lastLedgerSequence > 0) {
        fields[index].typeCode = 2;
        fields[index].fieldCode = 27;
        fields[index++].data.i32 = transaction->lastLedgerSequence;
    }

    return index;
}

static uint64_t calculateFee(BRRippleTransaction transaction)
{
    // We need to calculate the fee now.
    // TODO - we currently only support Payment transactions where
    // the fee = baseFee * numSignatures and since we only support a single
    // signature there is nothing to do here yet.
    // See https://xrpl.org/transaction-cost.html for the calculations required
    return transaction->feeBasis.pricePerCostFactor * transaction->feeBasis.costFactor;
}

/*
 * Serialize the transaction
 *
 * @return serializedTransaction  valid BRRippleSerializedTransaction handle OR
 *                                NULL if unable to serialize
 */
static BRRippleSerializedTransaction
rippleTransactionSerializeImpl (BRRippleTransaction transaction,
                            uint8_t *signature, int sig_length)
{
    assert(transaction);
    assert(transaction->transactionType == RIPPLE_TX_TYPE_PAYMENT);
    // NOTE - the address fields will hold a BRRippleAddress pointer BUT
    // they are owned by the the transaction or transfer so we don't need
    // to worry about the memory.
    BRRippleField fields[11];

    transaction->fee = calculateFee(transaction);
    int num_fields = setFieldInfo(fields, transaction, signature, sig_length);

    BRRippleSerializedTransaction signedBytes = NULL;

    int size = rippleSerialize(fields, num_fields, 0, 0);
    if (size > 0) {
        // I guess we will be sending back something
        // TODO validate the serialized bytes
        signedBytes = calloc(1, sizeof(struct BRRippleSerializedTransactionRecord));
        signedBytes->size = size + 512; // Allocate an extra 512 bytes for safety
        signedBytes->buffer = calloc(1, signedBytes->size);
        signedBytes->size = rippleSerialize(fields, num_fields, signedBytes->buffer, signedBytes->size);
        if (0 == signedBytes->size) {
            // Something bad happened - free memory and set pointer to NULL
            // LOG somewhere ???
            rippleSerializedTransactionRecordFree(&signedBytes);
            signedBytes = NULL;
        }
    }
    return signedBytes;
}

static void createTransactionHash(BRRippleSerializedTransaction signedBytes)
{
    assert(signedBytes);
    uint8_t bytes_to_hash[signedBytes->size + 4];

    // Add the transaction prefix before hashing
    bytes_to_hash[0] = 'T';
    bytes_to_hash[1] = 'X';
    bytes_to_hash[2] = 'N';
    bytes_to_hash[3] = 0;

    // Copy the rest of the bytes into the buffer
    memcpy(&bytes_to_hash[4], signedBytes->buffer, signedBytes->size);

    // Do a sha512 hash and use the first 32 bytes
    uint8_t md64[64];
    BRSHA512(md64, bytes_to_hash, sizeof(bytes_to_hash));
    memcpy(signedBytes->txHash, md64, 32);
}

extern size_t
rippleTransactionSerializeAndSign(BRRippleTransaction transaction, BRKey * privateKey,
                                  BRKey *publicKey, uint32_t sequence, uint32_t lastLedgerSequence)
{
    // If this transaction was previously signed - delete that info
    if (transaction->signedBytes) {
        free(transaction->signedBytes);
        transaction->signedBytes = 0;
    }

    // Add in the provided parameters
    transaction->sequence = sequence;
    transaction->lastLedgerSequence = lastLedgerSequence;
    
    // Add the public key to the transaction
    transaction->publicKey = *publicKey;
    
    // Serialize and sign the tx bytes
    BRRippleSerializedTransaction serializedBytes = rippleTransactionSerializeImpl (transaction, 0, 0);
    BRRippleSignature sig = signBytes(privateKey, serializedBytes->buffer, serializedBytes->size);

    transaction->signedBytes = rippleTransactionSerializeImpl(transaction, sig->signature, sig->sig_length);
    
    rippleSignatureDelete(sig);
    rippleSerializedTransactionRecordFree(&serializedBytes);

    // If we got a valid result then generate a hash
    if (transaction->signedBytes) {
        // Create and store a transaction hash of the transaction - the hash is attached to the signed
        // bytes object and will get destroyed if a subsequent serialization is done.
        createTransactionHash(transaction->signedBytes);
    }

    // Return the pointer to the signed byte object (or perhaps NULL)
    return (NULL == transaction->signedBytes ? 0 : transaction->signedBytes->size);
}

extern uint8_t* rippleTransactionSerialize(BRRippleTransaction transaction, size_t * bufferSize)
{
    assert(transaction);
    assert(bufferSize);
    // If we have serialized and signed this transaction then copy the bytes to the caller
    if (transaction->signedBytes) {
        uint8_t * buffer = calloc(1, transaction->signedBytes->size);
        memcpy(buffer, transaction->signedBytes->buffer, transaction->signedBytes->size);
        *bufferSize = transaction->signedBytes->size;
        return buffer;
    } else {
        // Not yet seralialize and signed
        *bufferSize = 0;
        return NULL;
    }
}

extern BRRippleTransactionHash rippleTransactionGetHash(BRRippleTransaction transaction)
{
    BRRippleTransactionHash hash;
    memset(hash.bytes, 0x00, sizeof(hash.bytes));

    // See if we have any signed bytes
    if (transaction->signedBytes) {
        memcpy(hash.bytes, transaction->signedBytes->txHash, 32);
    }

    return hash;
}

extern BRRippleTransactionHash rippleTransactionGetAccountTxnId(BRRippleTransaction transaction)
{
    assert(transaction);

    BRRippleTransactionHash hash;
    memset(hash.bytes, 0x00, sizeof(hash.bytes));

    // Copy whatever is in the field - might be nulls
    memcpy(hash.bytes, transaction->accountTxnID.bytes, 32);

    return hash;
}

extern BRRippleTransactionType rippleTransactionGetType(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->transactionType;
}

extern BRRippleUnitDrops rippleTransactionGetFee(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->fee; // Always XRP
}
extern BRRippleUnitDrops rippleTransactionGetAmount(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->payment.amount.amount.u64Amount; // XRP only
}
extern BRRippleSequence rippleTransactionGetSequence(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->sequence;
}
extern BRRippleFlags rippleTransactionGetFlags(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->flags;
}
extern BRRippleAddress rippleTransactionGetSource(BRRippleTransaction transaction)
{
    assert(transaction);
    return rippleAddressClone (transaction->sourceAddress);
}
extern BRRippleAddress rippleTransactionGetTarget(BRRippleTransaction transaction)
{
    assert(transaction);
    return rippleAddressClone (transaction->payment.targetAddress);
}

extern BRKey rippleTransactionGetPublicKey(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->publicKey;
}

extern BRRippleSignatureRecord rippleTransactionGetSignature(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->signature;
}

extern UInt256 rippleTransactionGetInvoiceID(BRRippleTransaction transaction)
{
    assert(transaction);
    UInt256 bytes;
    memset(bytes.u8, 0x00, sizeof(bytes.u8));
    memcpy(bytes.u8, transaction->payment.invoiceId, 32);
    return bytes;
}

extern void rippleTransactionSetInvoiceID (BRRippleTransaction transaction, UInt256 invoiceId) {
    memcpy (transaction->payment.invoiceId, invoiceId.u8, sizeof (transaction->payment.invoiceId));
}

extern BRRippleSourceTag rippleTransactionGetSourceTag(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->sourceTag;
}

extern BRRippleDestinationTag rippleTransactionGetDestinationTag(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->payment.destinationTag;
}

extern void rippleTransactionSetDestinationTag (BRRippleTransaction transaction, BRRippleDestinationTag tag) {
    transaction->payment.destinationTag = tag;
}

extern BRRippleLastLedgerSequence rippleTransactionGetLastLedgerSequence(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->lastLedgerSequence;
}

extern BRRippleAmount rippleTransactionGetAmountRaw(BRRippleTransaction transaction,
                                                    BRRippleAmountType amountType)
{
    switch(amountType) {
        case RIPPLE_AMOUNT_TYPE_AMOUNT:
            return transaction->payment.amount;
        case RIPPLE_AMOUNT_TYPE_SENDMAX:
            return transaction->payment.sendMax;
        case RIPPLE_AMOUNT_TYPE_DELIVERMIN:
            return transaction->payment.deliverMin;
        default:
        {
            // Invalid type - return an invalid amount object
            BRRippleAmount amount;
            amount.currencyType = -1;
            return amount;
            break;
        }
    }
}

BRRippleTransactionType mapTransactionType(uint16_t txType)
{
    if (txType <= 20 || txType == 100 || txType == 101) {
        return (BRRippleTransactionType)txType;
    }
    return RIPPLE_TX_TYPE_UNKNOWN;
}

void getFieldInfo(BRArrayOf(BRRippleField) fieldArray, int fieldCount, BRRippleTransaction transaction)
{
    for (int i = 0; i < fieldCount; i++) {
        BRRippleField *field = &fieldArray[i];
        switch(field->typeCode) {
            case 1:
                if (2 == field->fieldCode) {
                    // Map to our enum
                    transaction->transactionType = mapTransactionType(field->data.i16);
                }
                break;
            case 2:
                if (2 == field->fieldCode) {
                    transaction->flags = field->data.i32;
                } else if (3 == field->fieldCode) {
                    transaction->sourceTag = field->data.i32;
                } else if (4 == field->fieldCode) {
                    transaction->sequence = field->data.i32;
                } else if (14 == field->fieldCode) {
                    transaction->payment.destinationTag = field->data.i32;
                } else if (27 == field->fieldCode) {
                    transaction->lastLedgerSequence = field->data.i32;
                }
                break;
            case 5: // Hash256
                if (9 == field->fieldCode) {
                    memcpy(transaction->accountTxnID.bytes, field->data.hash, 32);
                } else if (17 == field->fieldCode) {
                    memcpy(transaction->payment.invoiceId, field->data.hash, 32);
                }
                break;
            case 6: // Amount object
                if (8 == field->fieldCode) { // fee)
                    transaction->fee = field->data.amount.amount.u64Amount;
                } else if (1 == field->fieldCode) { // amount
                    transaction->payment.amount = field->data.amount;
                } else if (9 == field->fieldCode) { // fee
                    transaction->payment.sendMax = field->data.amount;
                } else if (10 == field->fieldCode) {
                    transaction->payment.deliverMin = field->data.amount;
                }
            case 7: // Blob data
                if (3 == field->fieldCode) { // public key
                    transaction->publicKey = field->data.publicKey;
                } else if (4 == field->fieldCode) { // signature
                    transaction->signature = field->data.signature;
                }
                break;
            case 8: // Addresses - 20 bytes
                if (1 == field->fieldCode) { // source address
                    transaction->sourceAddress = field->data.address;
                } else if (3 == field->fieldCode) { // target address
                    transaction->payment.targetAddress = field->data.address;
                }
            default:
                break;
        }
    }
}

extern BRRippleTransaction
rippleTransactionCreateFromBytes(uint8_t *bytes, int length)
{
    BRArrayOf(BRRippleField) fieldArray;
    array_new(fieldArray, 15);
    // Since the BRArray memory might be moved to a realloc we need to
    // pass in the address of the array so it can be modified
    rippleDeserialize(bytes, length, &fieldArray);
    
    BRRippleTransaction transaction = createTransactionObject();

    int arrayCount = (int)array_count(fieldArray);
    getFieldInfo(fieldArray, arrayCount, transaction);

    // Before we get rid of the fields - see if there are any fields that
    // need to be cleaned up (i.e. they allocated some memory
    for (int i = 0; i < arrayCount; i++) {
        BRRippleField *field = &fieldArray[i];
        if (15 == field->typeCode && 9 == field->fieldCode) {
            // An array of Memos
            transaction->memos = field->memos;
        }
        // NOTE - the deserialization process created BRRippleAddress objects
        // but in the call above to getFieldInfo the transaction object now owns the memory
    }
    array_free(fieldArray);

    // Store the raw bytes as well
    transaction->signedBytes = calloc(1, sizeof(struct BRRippleSerializedTransactionRecord));
    transaction->signedBytes->buffer = calloc(1, length);
    memcpy(transaction->signedBytes->buffer, bytes, length);
    transaction->signedBytes->size = length;

    return transaction;
}

extern BRRippleFeeBasis rippleTransactionGetFeeBasis(BRRippleTransaction transaction)
{
    assert(transaction);
    return transaction->feeBasis;
}

static size_t
rippleTransactionHashValue (const void *h) {
    const BRRippleTransaction t = (BRRippleTransaction) h;
    UInt256 *hash = (UInt256*) t->signedBytes->txHash;
    return hash->u32[0];
}

static int
rippleTransactionHashEqual (const void *h1, const void *h2) {
    BRRippleTransaction t1 = (BRRippleTransaction) h1;
    BRRippleTransaction t2 = (BRRippleTransaction) h2;
    return h1 == h2 || 0 == memcmp (t1->signedBytes->txHash, t2->signedBytes->txHash, 32);
}

extern BRSetOf(BRRippleTransaction) rippleTransactionSetCreate (size_t initialSize) {
    return BRSetNew (rippleTransactionHashValue, rippleTransactionHashEqual, initialSize);
}

