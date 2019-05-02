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
#include "BRRippleSerialize.h"


typedef struct _txPaymentRecord {
    // The address to whom the payment is being sent
    BRRippleAddress targetAddress;

    // The payment amount (currently only supporting XRP drops
    uint64_t amount;
} BRRipplePaymentTxRecord;

struct BRRippleTransactionRecord {
    
    // COMMON FIELDS

    // The address of the account "doing" the transaction
    BRRippleAddress sourceAddress;

    // The Transaction type
    BRRippleTransactionType transactionType;

    // The transaction fee (in drops)
    uint64_t fee;
    
    // The next valid sequence number for the initiating account
    uint32_t sequence;

    uint32_t flags;
    uint32_t lastLedgerSequence;

    // The account
    BRKey publicKey;

    // The ripple payment information
    // TODO in the future if more transaction are supported this could
    // be changed to a union of the various types
    BRRipplePaymentTxRecord *payment;
};

extern BRRippleTransaction
rippleTransactionCreate(BRRippleAddress sourceAddress,
                        BRRippleAddress targetAddress,
                        BRRippleTransactionType txType,
                        uint64_t amount, // For now assume XRP drops.
                        uint32_t sequence,
                        uint64_t fee,
                        uint32_t flags,
                        uint32_t lastLedgerSequence,
                        BRKey publicKey)
{
    BRRippleTransaction transaction = calloc (1, sizeof (struct BRRippleTransactionRecord));
    
    assert(txType == PAYMENT);

    // Common fields
    transaction->fee = fee;
    transaction->sequence = sequence;
    transaction->sourceAddress = sourceAddress;
    transaction->transactionType = txType;
    transaction->publicKey = publicKey;
    transaction->flags = flags;
    transaction->lastLedgerSequence = lastLedgerSequence;

    // Payment information
    transaction->payment = calloc(1, sizeof(BRRipplePaymentTxRecord));
    transaction->payment->targetAddress = targetAddress;
    transaction->payment->amount = amount;
    
    return transaction;
}

extern void deleteRippleTransaction(BRRippleTransaction transaction)
{
    assert(transaction);
    if (transaction->payment) {
        free(transaction->payment);
    }
    free(transaction);
}

void setFieldInfo(BRRippleField *fields, int num_fields, BRRippleTransaction transaction,
                  uint8_t * signature, int sig_length)
{
    // Convert all the content to ripple fields
    fields[0].typeCode = 8;
    fields[0].fieldCode = 1;
    fields[0].data.address = transaction->sourceAddress;
    fields[1].typeCode = 1;
    fields[1].fieldCode = 2;
    fields[1].data.i16 = transaction->transactionType;
    fields[2].typeCode = 2;
    fields[2].fieldCode = 4;
    fields[2].data.i32 = transaction->sequence;
    fields[3].typeCode = 6;
    fields[3].fieldCode = 8;
    fields[3].data.i64 = transaction->fee;
    
    // Payment info
    fields[4].typeCode = 8;
    fields[4].fieldCode = 3;
    fields[4].data.address = transaction->payment->targetAddress;
    fields[5].typeCode = 6;
    fields[5].fieldCode = 1;
    fields[5].data.i64 = transaction->payment->amount;

    // Public key info
    fields[6].typeCode = 7;
    fields[6].fieldCode = 3;
    fields[6].data.publicKey = transaction->publicKey;

    fields[7].typeCode = 2;
    fields[7].fieldCode = 2;
    fields[7].data.i32 = transaction->flags;
    fields[8].typeCode = 2;
    fields[8].fieldCode = 27;
    fields[8].data.i32 = transaction->lastLedgerSequence;

    if (signature) {
        fields[9].typeCode = 7;
        fields[9].fieldCode = 4;
        memcpy(&fields[9].data.signature.signature, signature, sig_length);
        fields[9].data.signature.sig_length = sig_length;
    }
}

extern BRRippleSerializedTransaction
rippleTransactionSerialize (BRRippleTransaction transaction)
{
    assert(transaction);
    assert(transaction->transactionType == PAYMENT);
    assert(transaction->payment);
    BRRippleField fields[9];

    setFieldInfo(fields, 9, transaction, 0, 0);

    // Serialize the fields
    return(serialize(fields, 9));
}

extern BRRippleSerializedTransaction
rippleTransactionSerializeWithSignature(BRRippleTransaction transaction,
                                        uint8_t *signature,
                                        int    sig_length)
{
    assert(transaction);
    assert(transaction->transactionType == PAYMENT);
    assert(transaction->payment);
    BRRippleField fields[10];
    
    assert(signature);
    setFieldInfo(fields, 10, transaction, signature, sig_length);
    
    // Serialize the fields
    return(serialize(fields, 10));
}
