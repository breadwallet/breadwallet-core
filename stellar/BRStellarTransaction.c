//
//  BRStellarTransaction.h
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "BRStellar.h"
#include "BRStellarBase.h"
#include "BRStellarPrivateStructs.h"
#include "BRStellarSerialize.h"
#include "BRStellarSignature.h"
#include "BRStellarAccount.h"
#include "BRCrypto.h"
#include "BRArray.h"
#include "BRInt.h"

struct BRStellarSerializedTransactionRecord {
    size_t   size;
    uint8_t  *buffer;
    uint8_t  txHash[32];
};

struct BRStellarTransactionRecord {
    // The address of the account "doing" the transaction
    BRStellarAccountID accountID;
    BRStellarFee fee;
    BRStellarSequence sequence;
    BRStellarTimeBounds *timeBounds;
    int numTimeBounds;
    BRStellarMemo *memo;
    BRStellarOperation * operations;
    int numOperations;
    BRStellarSerializedTransaction signedBytes;
};

void stellarSerializedTransactionRecordFree(BRStellarSerializedTransaction * signedBytes)
{
    assert(signedBytes);
    assert(*signedBytes);
    if ((*signedBytes)->buffer) {
        free((*signedBytes)->buffer);
    }
    free(*signedBytes);
}

static BRStellarTransaction createTransactionObject(BRStellarAccountID *accountID,
                                                    BRStellarFee fee,
                                                    BRStellarTimeBounds *timeBounds,
                                                    int numTimeBounds,
                                                    BRStellarMemo *memo,
                                                    BRStellarOperation * operations,
                                                    int numOperations)
{
    BRStellarTransaction transaction = calloc (1, sizeof (struct BRStellarTransactionRecord));
    assert(transaction);
    transaction->accountID = *accountID;
    transaction->fee = fee;
    transaction->timeBounds = timeBounds;
    transaction->numTimeBounds = numTimeBounds;
    transaction->memo = memo;
    transaction->operations = operations;
    transaction->numOperations = numOperations;
    transaction->signedBytes = NULL;
    return transaction;
}

extern BRStellarTransaction
stellarTransactionCreate(BRStellarAccountID *accountID,
                         BRStellarFee fee,
                         BRStellarTimeBounds *timeBounds,
                         int numTimeBounds,
                         BRStellarMemo *memo,
                         BRStellarOperation * operations,
                         int numOperations)
{
    return createTransactionObject(accountID, fee, timeBounds, numTimeBounds,
                                memo, operations, numOperations);
}


extern void stellarTransactionFree(BRStellarTransaction transaction)
{
    assert(transaction);
    if (transaction->signedBytes) {
        stellarSerializedTransactionRecordFree(&transaction->signedBytes);
    }
    free(transaction);
}

/*
 * Serialize the transaction
 *
 * @return serializedTransaction  valid BRStellarSerializedTransaction handle OR
 *                                NULL if unable to serialize
 */
static BRStellarSerializedTransaction
stellarTransactionSerialize (BRStellarTransaction transaction,
                            uint8_t *signature, int sig_length)
{
    return NULL;
}

static void createTransactionHash(BRStellarSerializedTransaction signedBytes)
{
}

extern BRStellarSerializedTransaction
stellarTransactionSerializeAndSign(BRStellarTransaction transaction, uint8_t *privateKey,
                                  uint8_t *publicKey, uint64_t sequence)
{
    // If this transaction was previously signed - delete that info
    if (transaction->signedBytes) {
        stellarSerializedTransactionRecordFree(&transaction->signedBytes);
        transaction->signedBytes = 0;
    }
    
    // Add in the provided parameters
    transaction->sequence = sequence;

    // Serialize the bytes
    uint8_t * buffer = NULL;
    size_t length = stellarSerializeTransaction(&transaction->accountID, transaction->fee, sequence,
                                                transaction->timeBounds,
                                                transaction->numTimeBounds,
                                                transaction->memo,
                                                transaction->operations,
                                                transaction->numOperations,
                                                0, NULL, 0, &buffer);
    
    // Sign the bytes and get signature
    BRStellarSignatureRecord sig = stellarTransactionSign(buffer, length,
                                                          "Test SDF Network ; September 2015",
                                                          privateKey, publicKey);

    // Serialize the bytes
    free(buffer);
    length = stellarSerializeTransaction(&transaction->accountID, transaction->fee, sequence,
                                                transaction->timeBounds,
                                                transaction->numTimeBounds,
                                                transaction->memo,
                                                transaction->operations,
                                                transaction->numOperations,
                                                0, sig.signature, 68, &buffer);

    if (length) {
        transaction->signedBytes = calloc(1, sizeof(struct BRStellarSerializedTransactionRecord));
        transaction->signedBytes->buffer = calloc(1, length);
        memcpy(transaction->signedBytes->buffer, buffer, length);
        transaction->signedBytes->size = length;
    }
    // If we got a valid result then generate a hash
    if (transaction->signedBytes) {
        // Create and store a transaction hash of the transaction - the hash is attached to the signed
        // bytes object and will get destroyed if a subsequent serialization is done.
        createTransactionHash(transaction->signedBytes);
    }
    
    // Return the pointer to the signed byte object (or perhaps NULL)
    return transaction->signedBytes;

}

extern BRStellarTransactionHash stellarTransactionGetHash(BRStellarTransaction transaction)
{
    BRStellarTransactionHash hash;
    return hash;
}

extern BRStellarTransactionHash stellarTransactionGetAccountTxnId(BRStellarTransaction transaction)
{
    assert(transaction);
    BRStellarTransactionHash hash;
    return hash;
}

extern size_t stellarGetSerializedSize(BRStellarSerializedTransaction s)
{
    assert(s);
    return s->size;
}
extern uint8_t* stellarGetSerializedBytes(BRStellarSerializedTransaction s)
{
    assert(s);
    return (s->buffer);
}
