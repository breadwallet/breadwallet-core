//
//  testStellar.c
//  Core
//
//  Created by Carl Cherry on 5/21/2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "BRStellar.h"
#include "BRCrypto.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "BRKey.h"
#include "utils/b64.h"
#include "BRStellarSerialize.h"
#include "BRStellarTransaction.h"

static int debug_log = 1;

static uint8_t char2int(char input)
{
    if(input >= '0' && input <= '9')
        return input - '0';
    if(input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if(input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0;
}

static void hex2bin(const char* src, uint8_t * target)
{
    while(*src && src[1])
    {
        *(target++) = (char2int(src[0]) << 4) | (char2int(src[1]) & 0xff);
        src += 2;
    }
}

static BRStellarAccount createTestAccount(const char* paper_key,
                              const char* public_key_string, const char* expected_address)
{
    BRStellarAccount account = stellarAccountCreate(paper_key);
    
    uint8_t expected_public_key[32];
    hex2bin(public_key_string, expected_public_key);
    BRKey key = stellarAccountGetPublicKey(account);
    if (debug_log) {
        for (int i = 0; i < 32; i++) {
            printf("%02X", key.pubKey[i]);
        }
        printf("\n");
    }
    assert(0 == memcmp(key.pubKey, expected_public_key, sizeof(expected_public_key)));

    BRStellarAddress address = stellarAccountGetAddress(account);
    if (debug_log) printf("stellar address: %s\n", address.bytes);
    assert(0 == memcmp(address.bytes, expected_address, strlen(expected_address)));

    return account;
}

static void runAccountTests()
{
    // Test account 1
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    const char* public_key_string = "5562f344b6471448b7b6ebeb5bae9c1cecc930ef28868be2bb78bb742831e710";
    const char* expected_address = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    BRStellarAccount account = createTestAccount(paper_key, public_key_string, expected_address);
    stellarAccountFree(account);

    // Test account 2
    account = createTestAccount("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy",
                      "240FFEB7CF417181B0B0932035F8BC086B04D16C18B1DB8C629F1105E2687AD1",
                      "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    stellarAccountFree(account);
}

void decodeXRDPayment() {
    const char * xdr = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAyAAHHCYAAAAGAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAACAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAQAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAFVU0QAAAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAAA9ZI2AAAAAAAAAAAeJoetEAAABAg6x6wTbG9arAsdZ2SZ7hHcvY4rgCHgakJRMiOs2F4z/ruGW8UGPXNG9NkNDe6KbXMhwT5wI5cZTOJaS1PNdxBw==";

    size_t decodeSize = 0;
    unsigned char* decoded = b64_decode_ex(xdr, strlen(xdr), &decodeSize);
    for(int i = 0; i < decodeSize; i++) {
        if (i % 8 == 0) printf("\n");
        printf("%02X ", decoded[i]);
    }
    printf("\n");

    // Test uint64_t = 00071C26000000010000
    uint8_t buffer[] = {0x00, 0x07, 0x1C, 0x26, 0x00, 0x00, 0x00, 0x01};
    uint64_t num = ((uint64_t)buffer[0] << 56) +
    ((uint64_t)buffer[1] << 48) +
    ((uint64_t)buffer[2] << 40) +
    ((uint64_t)buffer[3] << 32) +
    (buffer[4] << 24) +
    (buffer[5] << 16) +
    (buffer[6] << 8) +
    buffer[7];
    printf("num: %llu\n", num);
}

static void serializeMinimum()
{
    const char* sourcePublicKeyString =
        "240FFEB7CF417181B0B0932035F8BC086B04D16C18B1DB8C629F1105E2687AD1";
    BRStellarAccountID sourceAccount;
    sourceAccount.accountType = PUBLIC_KEY_TYPE_ED25519;
    hex2bin(sourcePublicKeyString, sourceAccount.accountID);

    const char* targetPublicKeyString = "5562f344b6471448b7b6ebeb5bae9c1cecc930ef28868be2bb78bb742831e710";
    BRStellarAccountID targetAccount;
    targetAccount.accountType = PUBLIC_KEY_TYPE_ED25519;
    hex2bin(targetPublicKeyString, targetAccount.accountID);

    BRStellarMemo memo;
    memo.memoType = 1;
    strcpy(memo.text, "Buy yourself a beer!");
    BRStellarOperation * op1 = calloc(1, sizeof(BRStellarOperation));
    op1->type = PAYMENT;
    strcpy(op1->operation.payment.asset.assetCode, "XLM");
    op1->operation.payment.destination = targetAccount;
    op1->operation.payment.amount = 10.5;
    BRStellarOperation * op2 = calloc(1, sizeof(BRStellarOperation));
    op2->type = PAYMENT;
    op2->operation.payment.asset.type = 1;
    strcpy(op2->operation.payment.asset.assetCode, "USD");
    op2->operation.payment.asset.issuer = sourceAccount;
    op2->operation.payment.destination = targetAccount;
    op2->operation.payment.amount = 25.75;
    BRStellarOperation ops[] = { *op1, *op2 };
    
    uint8_t *buffer = NULL;
    size_t length = stellarSerializeTransaction(&sourceAccount, 200, 2001274371309571, NULL, 0,
                                &memo, ops, 2, 0, NULL, 0, &buffer);
    for(int i = 0; i < length; i++) {
        if (i % 8 == 0) printf("\n");
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    free(buffer);
}

static void serializeAndSign()
{
    const char* sourcePublicKeyString =
    "240FFEB7CF417181B0B0932035F8BC086B04D16C18B1DB8C629F1105E2687AD1";
    BRStellarAccountID sourceAccount;
    sourceAccount.accountType = PUBLIC_KEY_TYPE_ED25519;
    hex2bin(sourcePublicKeyString, sourceAccount.accountID);
    
    const char* targetPublicKeyString = "5562f344b6471448b7b6ebeb5bae9c1cecc930ef28868be2bb78bb742831e710";
    BRStellarAccountID targetAccount;
    targetAccount.accountType = PUBLIC_KEY_TYPE_ED25519;
    hex2bin(targetPublicKeyString, targetAccount.accountID);
    
    uint8_t buffer[1024];
    BRStellarMemo memo;
    memo.memoType = 1;
    strcpy(memo.text, "Buy yourself a beer!");
    BRStellarOperation * op1 = calloc(1, sizeof(BRStellarOperation));
    op1->type = PAYMENT;
    strcpy(op1->operation.payment.asset.assetCode, "XLM");
    op1->operation.payment.destination = targetAccount;
    op1->operation.payment.amount = 10.5;
    BRStellarOperation * op2 = calloc(1, sizeof(BRStellarOperation));
    op2->type = PAYMENT;
    op2->operation.payment.asset.type = 1;
    strcpy(op2->operation.payment.asset.assetCode, "USD");
    op2->operation.payment.asset.issuer = sourceAccount;
    op2->operation.payment.destination = targetAccount;
    op2->operation.payment.amount = 25.75;
    BRStellarOperation ops[] = { *op1, *op2 };
    
    BRStellarTransaction transaction = stellarTransactionCreate(&sourceAccount, 200, NULL, 0, &memo, ops, 2);
    BRStellarAccount account = stellarAccountCreate("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    stellarAccountSetSequence(account, 2001274371309574);
    BRStellarSerializedTransaction s = stellarAccountSignTransaction(account, transaction,
                                  "off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");

    size_t sSize = stellarGetSerializedSize(s);
    uint8_t *sBytes = stellarGetSerializedBytes(s);
    assert(sSize > 0);
    assert(sBytes);

    if (debug_log) {
        printf("sBytes: \n");
        for (int i = 0; i < sSize; i++) {
            if (i != 0 && i % 8 == 0) printf("\n");
            printf("%02X ", sBytes[i]);
        }
        printf("\n");
    }
}

void runSerializationTests()
{
    serializeMinimum();
    serializeAndSign();
}

extern void
runStellarTest (void /* ... */) {
    decodeXRDPayment();
    runAccountTests();
    runSerializationTests();
}
