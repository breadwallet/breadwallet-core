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

    if (public_key_string) {
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
    }

    BRStellarAddress address = stellarAccountGetAddress(account);
    if (debug_log) printf("stellar address: %s\n", address.bytes);
    if (expected_address) {
        assert(0 == memcmp(address.bytes, expected_address, strlen(expected_address)));
    }

    return account;
}

static void runAccountTests()
{
    // Test Account - first reference account from
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0005.md
    // illness spike retreat truth genius clock brain pass fit cave bargain toe
    BRStellarAccount  account = createTestAccount("illness spike retreat truth genius clock brain pass fit cave bargain toe", NULL,
        "GDRXE2BQUC3AZNPVFSCEZ76NJ3WWL25FYFK6RGZGIEKWE4SOOHSUJUJ6");
    stellarAccountFree(account);

    // Test Account - second reference account (15 words) from
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0005.md
    // illness spike retreat truth genius clock brain pass fit cave bargain toe
    account = createTestAccount("resource asthma orphan phone ice canvas fire useful arch jewel impose vague theory cushion top", NULL, "GAVXVW5MCK7Q66RIBWZZKZEDQTRXWCZUP4DIIFXCCENGW2P6W4OA34RH");
    stellarAccountFree(account);

    // Test Account - third reference account (24 words) from
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0005.md
    // illness spike retreat truth genius clock brain pass fit cave bargain toe
    account = createTestAccount("bench hurt jump file august wise shallow faculty impulse spring exact slush thunder author capable act festival slice deposit sauce coconut afford frown better", NULL, "GC3MMSXBWHL6CPOAVERSJITX7BH76YU252WGLUOM5CJX3E7UCYZBTPJQ");
    stellarAccountFree(account);

    // Account we use for sending on TESTNET
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    const char* public_key_string = "5562f344b6471448b7b6ebeb5bae9c1cecc930ef28868be2bb78bb742831e710";
    const char* expected_address = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    account = createTestAccount(paper_key, public_key_string, expected_address);
    stellarAccountFree(account);

    // Account we use for receiving on TESTNET
    account = createTestAccount("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy",
                      "240FFEB7CF417181B0B0932035F8BC086B04D16C18B1DB8C629F1105E2687AD1",
                      "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    stellarAccountFree(account);
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
    if (debug_log) {
        for(int i = 0; i < length; i++) {
            if (i % 8 == 0) printf("\n");
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
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
    BRStellarOperation ops[] = { *op1 };

    uint32_t fee = 100 * (sizeof(ops)/sizeof(BRStellarOperation));
    BRStellarTransaction transaction = stellarTransactionCreate(&sourceAccount, fee, NULL, 0, &memo, ops, 1);
    BRStellarAccount account = stellarAccountCreate("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    stellarAccountSetSequence(account, 2001274371309574);
    stellarAccountSetNetworkType(account, STELLAR_NETWORK_TESTNET);
    BRStellarSerializedTransaction s = stellarAccountSignTransaction(account, transaction,
                                  "off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");

    size_t sSize = stellarGetSerializedSize(s);
    uint8_t *sBytes = stellarGetSerializedBytes(s);
    assert(sSize > 0);
    assert(sBytes);

    // Base64 the bytes
    char * encoded = b64_encode(sBytes, sSize);
    if (debug_log) {
        printf("encoded bytes: %s\n", encoded);
        printf("sBytes: \n");
        for (int i = 0; i < sSize; i++) {
            if (i != 0 && i % 8 == 0) printf("\n");
            printf("%02X ", sBytes[i]);
        }
        printf("\n");
    }
    // Compare with what we are expecting
    const char* expected_b64 = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAGAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABA7SA5lCfGXhKqo44uczRi9kIIOVaAv02ugAIWK8vxVDDPk5zvjIbffBTDOhJpaf4kxnvsar7NWVHhsd+ieIyYCQ==";
    assert(0 == memcmp(encoded, expected_b64, strlen(expected_b64)));
    assert(strlen(encoded) == strlen(expected_b64));
    free(encoded);
}

void runSerializationTests()
{
    serializeMinimum();
    serializeAndSign();
}

extern void
runStellarTest (void /* ... */) {
    runAccountTests();
    runSerializationTests();
}
