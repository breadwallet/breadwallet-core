//
//  BRHederaSerialize.c
//
//  Created by Carl Cherry on Oct. 17, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include "BRHederaSerialize.h"
#include "proto/Transaction.pb-c.h"
#include "proto/TransactionBody.pb-c.h"
#include <stdlib.h>

Proto__AccountID * createAccountID (BRHederaAddress address)
{
    Proto__AccountID *protoAccountID = calloc(1, sizeof(Proto__AccountID));
    proto__account_id__init(protoAccountID);
    protoAccountID->shardnum = address.shard;
    protoAccountID->realmnum = address.realm;
    protoAccountID->accountnum = address.account;
    return protoAccountID;
}

Proto__Timestamp * createTimeStamp  (BRHederaTimeStamp timeStamp)
{
    Proto__Timestamp *ts = calloc(1, sizeof(Proto__Timestamp));
    proto__timestamp__init(ts);
    ts->seconds = timeStamp.seconds;
    ts->nanos = timeStamp.nano;
    return ts;
}

Proto__TransactionID * createTransactionID (BRHederaAddress address, BRHederaTimeStamp timeStamp)
{
    Proto__TransactionID *txID = calloc(1, sizeof(Proto__TransactionID));
    proto__transaction_id__init(txID);
    txID->transactionvalidstart = createTimeStamp(timeStamp);
    txID->accountid = createAccountID(address);

    return txID;
}

Proto__Duration * createTransactionDuration(int64_t seconds)
{
    Proto__Duration * duration = calloc(1, sizeof(Proto__Duration));
    proto__duration__init(duration);
    duration->seconds = seconds;
    return duration;
}

Proto__AccountAmount * createAccountAmount (BRHederaAddress address, int64_t amount)
{
    Proto__AccountAmount * accountAmount = calloc(1, sizeof(Proto__AccountAmount));
    proto__account_amount__init(accountAmount);
    accountAmount->accountid = createAccountID(address);
    accountAmount->amount = amount;
    return accountAmount;
}

uint8_t * hederaTransactionBodyPack (BRHederaAddress source,
                                       BRHederaAddress target,
                                       BRHederaAddress nodeAddress,
                                       BRHederaUnitTinyBar amount,
                                       BRHederaTimeStamp timeStamp,
                                       BRHederaUnitTinyBar fee,
                                       const char * memo,
                                       size_t *size)
{
    Proto__TransactionBody *body = calloc(1, sizeof(Proto__TransactionBody));
    proto__transaction_body__init(body);

    // Create a transaction ID
    body->transactionid = createTransactionID(source, timeStamp);
    body->nodeaccountid = createAccountID(nodeAddress);
    body->transactionfee = fee;
    // Set the duration
    body->transactionvalidduration = createTransactionDuration(120);
    // For this case we will add 3 transfers
    body->data_case =  PROTO__TRANSACTION_BODY__DATA_CRYPTO_TRANSFER;
    body->cryptotransfer = calloc(1, sizeof(Proto__CryptoTransferTransactionBody));
    proto__crypto_transfer_transaction_body__init(body->cryptotransfer);
    body->cryptotransfer->transfers = calloc(1, sizeof(Proto__TransferList));
    proto__transfer_list__init(body->cryptotransfer->transfers);
    // Create 3 transfers
    body->cryptotransfer->transfers->n_accountamounts = 2;
    body->cryptotransfer->transfers->accountamounts = calloc(2, sizeof(Proto__AccountAmount*));
    body->cryptotransfer->transfers->accountamounts[0] = createAccountAmount(source, -(amount));
    body->cryptotransfer->transfers->accountamounts[1] = createAccountAmount(target, amount);

    *size = proto__transaction_body__get_packed_size(body);
    uint8_t * buffer = calloc(1, *size);
    proto__transaction_body__pack(body, buffer);

    proto__transaction_body__free_unpacked(body, NULL);

    return buffer;
}

Proto__SignatureMap * createSigMap(uint8_t *signature, uint8_t * publicKey)
{
    Proto__SignatureMap * sigMap = calloc(1, sizeof(Proto__SignatureMap));
    proto__signature_map__init(sigMap);
    sigMap->sigpair = calloc(1, sizeof(Proto__SignaturePair*)); // A single signature
    sigMap->sigpair[0] = calloc(1, sizeof(Proto__SignaturePair));
    proto__signature_pair__init(sigMap->sigpair[0]);
    sigMap->sigpair[0]->signature_case = PROTO__SIGNATURE_PAIR__SIGNATURE_ED25519;
    sigMap->sigpair[0]->pubkeyprefix.data = publicKey;
    sigMap->sigpair[0]->pubkeyprefix.len = 32;
    sigMap->sigpair[0]->ed25519.data = signature;
    sigMap->sigpair[0]->ed25519.len = 64;
    sigMap->n_sigpair = 1;
    return sigMap;
}

uint8_t * hederaTransactionPack (uint8_t * signature, size_t signatureSize,
                                      uint8_t * publicKey, size_t publicKeySize,
                                      uint8_t * body, size_t bodySize,
                                      size_t * serializedSize)
{
    struct _Proto__Transaction * transaction = calloc(1, sizeof(struct _Proto__Transaction));
    proto__transaction__init(transaction);
    transaction->body_data_case = PROTO__TRANSACTION__BODY_DATA_BODY_BYTES;

    // Attach the signature and the bytes to our transaction object
    transaction->sigmap = createSigMap(signature, publicKey);
    transaction->bodybytes.data = body;
    transaction->bodybytes.len = bodySize;
    transaction->body_data_case = PROTO__TRANSACTION__BODY_DATA_BODY_BYTES;
    *serializedSize = proto__transaction__get_packed_size(transaction);
    uint8_t * serializeBytes = calloc(1, *serializedSize);
    proto__transaction__pack(transaction, serializeBytes);
    return serializeBytes;
}
