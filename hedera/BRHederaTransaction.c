/**
*/

#include "BRHederaTransaction.h"
#include "BRHederaCrypto.h"
#include "BRHederaSerialize.h"
#include "BRHederaUtils.h"
#include "proto/Transaction.pb-c.h"
#include "proto/TransactionBody.pb-c.h"
#include "vendor/ed25519/ed25519.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct BRHederaTransactionRecord {
    BRHederaAddress source;
    BRHederaAddress target;
    BRHederaUnitTinyBar amount;
    BRHederaTransactionId txId;
    BRHederaUnitTinyBar fee;
    uint8_t * serializedBytes;
    size_t serializedSize;
    BRHederaTransactionHash hash;
};

extern BRHederaTransaction hederaTransactionCreateNew (BRHederaAddress source,
                                                       BRHederaAddress target,
                                                       BRHederaUnitTinyBar amount)
{
    BRHederaTransaction transaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    transaction->serializedSize = 0;
    transaction->serializedBytes = NULL;

    transaction->source = source;
    transaction->target = target;
    transaction->amount = amount;

    return transaction;
}

extern BRHederaTransaction hederaTransactionCreate (BRHederaAddress source,
                                                    BRHederaAddress target,
                                                    BRHederaUnitTinyBar amount,
                                                    const char * txID,
                                                    BRHederaTransactionHash hash)
{
    // This is an existing transaction - it must have a transaction ID
    assert(txID);
    BRHederaTransaction transaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    transaction->serializedSize = 0;
    transaction->serializedBytes = NULL;

    transaction->source = source;
    transaction->target = target;
    transaction->amount = amount;

    // Parse the transactionID
    transaction->txId = hederaParseTransactionId(txID);
    transaction->hash = hash;

    return transaction;
}

extern void hederaTransactionFree (BRHederaTransaction transaction)
{
    assert (transaction);
    if (transaction->serializedBytes) free (transaction->serializedBytes);
    free (transaction);
}

extern size_t
hederaTransactionSignTransaction (BRHederaTransaction transaction,
                                  BRKey publicKey,
                                  BRHederaAddress nodeAddress,
                                  BRHederaTimeStamp timeStamp,
                                  BRHederaUnitTinyBar fee,
                                  UInt512 seed)
{
    assert(transaction);

    // Generate the private key from the seed
    BRKey key = hederaKeyCreate(seed);
    unsigned char privateKey[64] = {0};
    unsigned char temp[32] = {0}; // Use the public key that is sent in instead
    ed25519_create_keypair(temp, privateKey, key.secret.u8);

    // First we need to serialize the body since it is the thing we sign
    size_t bodySize;
    uint8_t * body = hederaTransactionBodyPack (transaction->source,
                                                transaction->target,
                                                nodeAddress,
                                                transaction->amount,
                                                timeStamp,
                                                fee,
                                                NULL,
                                                &bodySize);

    // Create signature from the body bytes
    unsigned char signature[64];
    memset(signature, 0x00, 64);
    ed25519_sign(signature, body, bodySize, publicKey.pubKey, privateKey);

    // Serialize the full transaction including signature and public key
    transaction->serializedBytes = hederaTransactionPack (signature, 64,
                                                          publicKey.pubKey, 32,
                                                          body, bodySize,
                                                          &transaction->serializedSize);
    return transaction->serializedSize;
}

extern uint8_t * hederaTransactionSerialize (BRHederaTransaction transaction, size_t *size)
{
    if (transaction->serializedBytes) {
        *size = transaction->serializedSize;
        return transaction->serializedBytes;
    } else {
        *size = 0;
        return NULL;
    }
}

extern BRHederaTransactionHash hederaTransactionGetHash(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->hash;
}

extern BRHederaTransactionId hederaTransactionGetTransactionId(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->txId;
}

extern BRHederaUnitTinyBar hederaTransactionGetFee(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->fee;
}

extern BRHederaUnitTinyBar hederaTransactionGetAmount(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->amount;
}

extern BRHederaAddress hederaTransactionGetSource(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->source;
}

extern BRHederaAddress hederaTransactionGetTarget(BRHederaTransaction transaction)
{
    assert(transaction);
    return transaction->target;
}
