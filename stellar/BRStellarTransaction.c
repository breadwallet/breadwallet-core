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

struct BRStellarTransactionRecord {
    // The address of the account "doing" the transaction
    BRStellarAddress sourceAddress;
};

struct BRStellarSerializedTransactionRecord {
    uint32_t size;
    uint8_t  *buffer;
    uint8_t  txHash[32];
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

static BRStellarTransaction createTransactionObject()
{
    BRStellarTransaction transaction = calloc (1, sizeof (struct BRStellarTransactionRecord));
    assert(transaction);
    return transaction;
}

extern BRStellarTransaction
stellarTransactionCreate(BRStellarAddress sourceAddress,
                        BRStellarAddress targetAddress)
{
    BRStellarTransaction transaction = createTransactionObject();
    return transaction;
}


extern void stellarTransactionFree(BRStellarTransaction transaction)
{
    assert(transaction);
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
stellarTransactionSerializeAndSign(BRStellarTransaction transaction, BRKey * privateKey,
                                  BRKey *publicKey, uint32_t sequence, uint32_t lastLedgerSequence)
{
    return NULL;
}

extern uint32_t getSerializedSize(BRStellarSerializedTransaction s)
{
    return s->size;
}
extern uint8_t* getSerializedBytes(BRStellarSerializedTransaction s)
{
    return s->buffer;
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
