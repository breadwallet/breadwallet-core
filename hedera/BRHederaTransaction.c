/**
*/

#include "BRHederaTransaction.h"
#include "BRHederaCrypto.h"
#include "proto/Transaction.pb-c.h"
#include "proto/TransactionBody.pb-c.h"
#include "vendor/ed25519/ed25519.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct BRHederaTransactionRecord {
    struct _Proto__Transaction transaction;
    Proto__TransactionBody     txBody;
};


Proto__AccountID * createAccountID (uint64_t accountNum)
{
    Proto__AccountID *accountID = calloc(1, sizeof(Proto__AccountID));
    proto__account_id__init(accountID);
    accountID->shardnum = 0;
    accountID->shardnum = 0;
    accountID->accountnum = accountNum;
    return accountID;
}

Proto__Timestamp * createTimeStamp  (void)
{
    Proto__Timestamp *ts = calloc(1, sizeof(Proto__Timestamp));
    proto__timestamp__init(ts);
    struct timespec currentTime;
    clock_gettime(CLOCK_REALTIME, &currentTime);

    //ts->seconds = currentTime.tv_sec;
    //ts->nanos = currentTime.tv_nsec * 1e-6;
    ts->seconds = 0;
    ts->nanos = 0;
    return ts;
}

Proto__TransactionID * createTransactionID (uint64_t accountNum)
{
    Proto__TransactionID *txID = calloc(1, sizeof(Proto__TransactionID));
    proto__transaction_id__init(txID);
    txID->transactionvalidstart = createTimeStamp();
    txID->accountid = createAccountID(accountNum);

    return txID;
}

Proto__AccountAmount * createAccountAmount (uint64_t accountNum, int64_t amount)
{
    Proto__AccountAmount * accountAmount = calloc(1, sizeof(Proto__AccountAmount));
    proto__account_amount__init(accountAmount);
    accountAmount->accountid = createAccountID(accountNum);
    accountAmount->amount = amount;
    return accountAmount;
}

Proto__Duration * createTransactionDuration(int64_t seconds)
{
    Proto__Duration * duration = calloc(1, sizeof(Proto__Duration));
    proto__duration__init(duration);
    duration->seconds = seconds;
    return duration;
}

void createCryptoTransaction (Proto__TransactionBody * txBody)
{
    proto__transaction_body__init(txBody);
    // Create a transaction ID
    txBody->transactionid = createTransactionID(2);
    txBody->nodeaccountid = createAccountID(2);
    txBody->transactionfee = 100000;
    // Set the duration
    txBody->transactionvalidduration = createTransactionDuration(120);
    // For this case we will add 3 transfers
    txBody->data_case =  PROTO__TRANSACTION_BODY__DATA_CRYPTO_TRANSFER;
    txBody->cryptotransfer = calloc(1, sizeof(Proto__CryptoTransferTransactionBody));
    proto__crypto_transfer_transaction_body__init(txBody->cryptotransfer);
    txBody->cryptotransfer->transfers = calloc(1, sizeof(Proto__TransferList));
    proto__transfer_list__init(txBody->cryptotransfer->transfers);
    // Create 3 transfers
    txBody->cryptotransfer->transfers->n_accountamounts = 3;
    txBody->cryptotransfer->transfers->accountamounts = calloc(3, sizeof(Proto__AccountAmount*));
    txBody->cryptotransfer->transfers->accountamounts[0] = createAccountAmount(4, -800);
    txBody->cryptotransfer->transfers->accountamounts[1] = createAccountAmount(55, 400);
    txBody->cryptotransfer->transfers->accountamounts[2] = createAccountAmount(78, 400);
}

extern BRHederaTransaction hederaTransactionCreate (void)
{
    BRHederaTransaction transaction = calloc(1, sizeof(struct BRHederaTransactionRecord));
    proto__transaction__init(&transaction->transaction);
    transaction->transaction.body_data_case = PROTO__TRANSACTION__BODY_DATA_BODY_BYTES;
    createCryptoTransaction (&transaction->txBody);
    
    return transaction;
}

extern size_t hederaTransactionSign (BRHederaTransaction transaction, UInt512 seed)
{
    BRKey key = hederaKeyCreate(seed);
    unsigned char privateKey[64] = {0};
    unsigned char publicKey[32] = {0};
    ed25519_create_keypair(publicKey, privateKey, key.secret.u8);

    size_t size = proto__transaction_body__get_packed_size(&transaction->txBody);
    uint8_t buffer[size];
    memset(&buffer[0], 0x00, size);

    proto__transaction_body__pack(&transaction->txBody, buffer);
    for (int i = 0; i < size; i++) {
        printf("%d = %d \n", i, (int8_t)buffer[i]);
    }
    printf("\n");
    // Now print it as a hex string
    for (int i = 0; i < size; i++) {
        printf("%02X", buffer[i]);
    }
    printf("\n");

    // Sign the packed bytes
    unsigned char signature[64];
    memset(signature, 0x00, 64);
    ed25519_sign(signature, buffer, size, publicKey, privateKey);
    printf("signature\n");
    for (int i = 0; i < 64; i++) {
        printf("%d = %d \n", i, (int8_t)signature[i]);
    }
    printf("\n");

    //proto__crypto_transfer_transaction_body__pack_to_buffer(transaction->txBody.cryptotransfer, &cBuffer);

    return size;
}


