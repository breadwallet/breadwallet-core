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
#include "support/BRCrypto.h"
#include "support/BRInt.h"

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
    uint32_t numTimeBounds;
    BRStellarMemo *memo;
    BRStellarOperation * operations;
    uint32_t numOperations;
    BRStellarSerializedTransaction signedBytes;
    uint32_t numSignatures;
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
    transaction->numSignatures = 0;
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

extern BRStellarTransaction /* caller must free - stellarTransactionFree */
stellarTransactionCreateFromBytes(uint8_t *bytes, size_t length)
{
    BRStellarTransaction transaction = calloc (1, sizeof (struct BRStellarTransactionRecord));

    int32_t version = 0;
    uint8_t *signatures = NULL;
    if (stellarDeserializeTransaction(&transaction->accountID,
                                       &transaction->fee,
                                       &transaction->sequence,
                                       &transaction->timeBounds,
                                       &transaction->numOperations,
                                       &transaction->memo,
                                       &transaction->operations,
                                       &transaction->numOperations,
                                       &version,
                                       &signatures,
                                       &transaction->numSignatures,
                                       bytes, length))
    {
        printf("Signature: \n");
        for(int i = 0; i < 68; i++) {
            printf("%02X ", signatures[i]);
        }
        printf("\n");
        return transaction;
    } else {
        stellarTransactionFree(transaction);
        return NULL;
    }
}

extern void stellarTransactionFree(BRStellarTransaction transaction)
{
    assert(transaction);
    if (transaction->signedBytes) {
        stellarSerializedTransactionRecordFree(&transaction->signedBytes);
    }
    free(transaction);
}

static void createTransactionHash(uint8_t *md32, uint8_t *tx, size_t txLength, const char* networkID)
{
    // What are we going to hash
    // sha256(networkID) + tx_type + tx
    // tx_type is basically a 4-byte packed int
    size_t size = 32 + 4 + txLength;
    uint8_t bytes_to_hash[size];
    uint8_t *pHash = bytes_to_hash;
    
    // Hash the networkID
    uint8_t networkHash[32];
    BRSHA256(networkHash, networkID, strlen(networkID));
    memcpy(pHash, networkHash, 32);
    pHash += 32;
    uint8_t tx_type[4] = {0, 0, 0, 2}; // Add the tx_type
    memcpy(pHash, tx_type, 4);
    pHash += 4;
    memcpy(pHash, tx, txLength); // Add the serialized transaction
    
    // Do a sha256 hash of the data
    BRSHA256(md32, bytes_to_hash, size);
}

// Map the network types to a string - get's hashed into the transaction
const char *stellarNetworks[] = {
    "Public Global Stellar Network ; September 2015",
    "Test SDF Network ; September 2015"
};

extern BRStellarSerializedTransaction
stellarTransactionSerializeAndSign(BRStellarTransaction transaction, uint8_t *privateKey,
                                  uint8_t *publicKey, uint64_t sequence, BRStellarNetworkType networkType)
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

    // Create the transaction hash that needs to be signed
    uint8_t tx_hash[32];
    createTransactionHash(tx_hash, buffer, length, stellarNetworks[networkType]);

    // Sign the bytes and get signature
    BRStellarSignatureRecord sig = stellarTransactionSign(tx_hash, 32, privateKey, publicKey);

    // Serialize the bytes and sign
    free(buffer);
    length = stellarSerializeTransaction(&transaction->accountID, transaction->fee, sequence,
                                                transaction->timeBounds,
                                                transaction->numTimeBounds,
                                                transaction->memo,
                                                transaction->operations,
                                                transaction->numOperations,
                                                0, sig.signature, 1, &buffer);

    if (length) {
        transaction->signedBytes = calloc(1, sizeof(struct BRStellarSerializedTransactionRecord));
        transaction->signedBytes->buffer = calloc(1, length);
        memcpy(transaction->signedBytes->buffer, buffer, length);
        transaction->signedBytes->size = length;
        memcpy(transaction->signedBytes->txHash, tx_hash, 32);
    }
    
    // Return the pointer to the signed byte object (or perhaps NULL)
    return transaction->signedBytes;

}

extern BRStellarTransactionHash stellarTransactionGetHash(BRStellarTransaction transaction)
{
    assert(transaction);
    BRStellarTransactionHash hash;
    if (transaction->signedBytes) {
        memcpy(hash.bytes, transaction->signedBytes->txHash, 32);
    } else {
        memset(hash.bytes, 0x00, 32);
    }
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

extern BRStellarAccountID stellarTransactionGetAccountID(BRStellarTransaction transaction)
{
    assert(transaction);
    return transaction->accountID;
}

extern uint32_t stellarTransactionGetOperationCount(BRStellarTransaction transaction)
{
    assert(transaction);
    return transaction->numOperations;
}
extern uint32_t stellarTransactionGetSignatureCount(BRStellarTransaction transaction)
{
    assert(transaction);
    return transaction->numSignatures;
}

