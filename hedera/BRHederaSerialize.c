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
#include "BRHederaAddress.h"
#include "proto/Transaction.pb-c.h"
#include "proto/TransactionBody.pb-c.h"
#include <stdlib.h>

Proto__AccountID * createAccountID (BRHederaAddress address)
{
    Proto__AccountID *protoAccountID = calloc(1, sizeof(Proto__AccountID));
    proto__account_id__init(protoAccountID);
    protoAccountID->shardnum = hederaAddressGetShard (address);
    protoAccountID->realmnum = hederaAddressGetRealm (address);
    protoAccountID->accountnum = hederaAddressGetAccount (address);
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

Proto__TransactionID * createProtoTransactionID (BRHederaAddress address, BRHederaTimeStamp timeStamp)
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
    body->transactionid = createProtoTransactionID(source, timeStamp);
    body->nodeaccountid = createAccountID(nodeAddress);
    body->transactionfee = fee;
    // Set the duration
    // *** NOTE 1 *** if the transaction is unable to be verified in this
    // duration then it will fail. The default value in the Hedera Java SDK
    // is 120. I have set ours to 180 since it requires a couple of extra hops
    // *** NOTE 2 *** if you change this value then it will break the unit tests
    // since it will change the serialized bytes.
    body->transactionvalidduration = createTransactionDuration(180);

    // We are creating a "Cryto Transfer" transaction which has a transfer list
    body->data_case =  PROTO__TRANSACTION_BODY__DATA_CRYPTO_TRANSFER;
    body->cryptotransfer = calloc(1, sizeof(Proto__CryptoTransferTransactionBody));
    proto__crypto_transfer_transaction_body__init(body->cryptotransfer);
    body->cryptotransfer->transfers = calloc(1, sizeof(Proto__TransferList));
    proto__transfer_list__init(body->cryptotransfer->transfers);

    // We are only supporting sending from A to B at this point - so create 2 transfers
    body->cryptotransfer->transfers->n_accountamounts = 2;
    body->cryptotransfer->transfers->accountamounts = calloc(2, sizeof(Proto__AccountAmount*));
    // NOTE - the amounts in the transfer MUST add up to 0
    body->cryptotransfer->transfers->accountamounts[0] = createAccountAmount(source, -(amount));
    body->cryptotransfer->transfers->accountamounts[1] = createAccountAmount(target, amount);

    // Serialize the transaction body
    *size = proto__transaction_body__get_packed_size(body);
    uint8_t * buffer = calloc(1, *size);
    proto__transaction_body__pack(body, buffer);

    // Free the body object now that we have serialized to bytes
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

    // We need to take copies of the the following fields or else we
    // get a double free assert.  They will get freed later in the call
    // to proto__transaction__free_unpacked
    uint8_t * pubKeyBuffer = calloc(1, 32);
    memcpy(pubKeyBuffer, publicKey, 32);
    uint8_t * sigCopy = calloc(1, 64);
    memcpy(sigCopy, signature, 64);

    sigMap->sigpair[0]->pubkeyprefix.data = pubKeyBuffer;
    sigMap->sigpair[0]->pubkeyprefix.len = 32;
    sigMap->sigpair[0]->ed25519.data = sigCopy;
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

    // The call to free_unpacked below will delete the .data field, so take
    // a copy here so we don't get a double free
    uint8_t * bodyCopy = calloc(1, bodySize);
    memcpy(bodyCopy, body, bodySize);
    transaction->bodybytes.data = bodyCopy;
    transaction->bodybytes.len = bodySize;
    transaction->body_data_case = PROTO__TRANSACTION__BODY_DATA_BODY_BYTES;

    // Get the packed bytes
    *serializedSize = proto__transaction__get_packed_size(transaction);
    uint8_t * serializeBytes = calloc(1, *serializedSize);
    proto__transaction__pack(transaction, serializeBytes);

    // Free the transaction now that we have serialized to bytes
    proto__transaction__free_unpacked(transaction, NULL);

    return serializeBytes;
}
