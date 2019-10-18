/**
*/

#include "BRHederaTransaction.h"
#include "BRHederaCrypto.h"
#include "BRHederaSerialize.h"
#include "proto/Transaction.pb-c.h"
#include "proto/TransactionBody.pb-c.h"
#include "vendor/ed25519/ed25519.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct BRHederaTransactionRecord {
    BRHederaAccountID source;
    BRHederaAccountID target;
    BRHederaUnitTinyBar amount;
    uint8_t * serializedBytes;
    size_t serializedSize;
};

extern BRHederaTransaction hederaTransactionCreate (BRHederaAccountID source, BRHederaAccountID target, BRHederaUnitTinyBar amount)
{
    BRHederaTransaction transaction = calloc (1, sizeof(struct BRHederaTransactionRecord));
    transaction->serializedSize = 0;
    transaction->serializedBytes = NULL;

    transaction->source = source;
    transaction->target = target;
    transaction->amount = amount;

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
                                  BRHederaAccountID nodeAccountID,
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
                                                nodeAccountID,
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
