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
#include "BRStellarAccountUtils.h"

static int debug_log = 0;

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
        *(target++) = (char2int(src[0]) << 4) | (char2int(src[1]) & 0x0f);
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
    stellarAccountSetSequence(account, 2001274371309576);
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
    const char* expected_b64 = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABAzBQpbrqpbfFozHnwpIATkErUPcb5xesMeFClf5dyd4X0kBw3c6gZUVTtHh3iCZ6eUAEge/lCft6NfXzsHy1HBQ==";
#if 0
    "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAGAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABA7SA5lCfGXhKqo44uczRi9kIIOVaAv02ugAIWK8vxVDDPk5zvjIbffBTDOhJpaf4kxnvsar7NWVHhsd+ieIyYCQ==";
#endif
    assert(0 == memcmp(encoded, expected_b64, strlen(expected_b64)));
    assert(strlen(encoded) == strlen(expected_b64));
    free(encoded);
}

void runSerializationTests()
{
    serializeMinimum();
    serializeAndSign();
}

static void testDeserialize(const char * input,
                            uint32_t expectedTxCount,
                            uint32_t expectedSignatureCount,
                            const char* accountString)
{
    // Turn the base64 into bytes
    size_t byteSize = 0;
    uint8_t * bytes = b64_decode_ex(input, strlen(input), &byteSize);
    for(int i = 0; i < byteSize; i++) {
        if (i >= 0 && i % 8 == 0) printf("\n");
        printf("%02X ", bytes[i]);
    }
    printf("\n");

    BRStellarTransaction transaction = stellarTransactionCreateFromBytes(&bytes[0], byteSize);
    assert(transaction);
    if (accountString) {
        BRStellarAccountID accountID = stellarTransactionGetAccountID(transaction);
        BRKey key;
        memcpy(key.pubKey, accountID.accountID, 32);
        BRStellarAddress address = createStellarAddressFromPublicKey(&key);
        assert(0 == strcmp(&address.bytes[0], accountString));
    }
    uint32_t txCount = stellarTransactionGetOperationCount(transaction);
    assert(txCount == expectedTxCount);
    uint32_t sigCount = stellarTransactionGetSignatureCount(transaction);
    assert(sigCount == expectedSignatureCount);
}

static const char* tx_one = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAGAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABA7SA5lCfGXhKqo44uczRi9kIIOVaAv02ugAIWK8vxVDDPk5zvjIbffBTDOhJpaf4kxnvsar7NWVHhsd+ieIyYCQ==";

static const char* tx_two = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAHAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABAlLvA6YjDlERdXd1gU5VYeczu26F+Wgt0VpGsfqdN0kgUx1B7GFdmB2tT2tKM72XLYu7Y2M6+c5QiDueVNP45BQ==";

static const char* tx_three = "AAAAAFF+B9zBBP1YlsE7qH3fgzFgDFqroQL9jk7rbFuEXrs1AAAD6AAIMqMAAALHAAAAAAAAAAEAAAATMTU1OTc3NTY0NDM4NzY2MzMxNwAAAAAKAAAAAAAAAAEAAAAA6Twf5NVbRbK9xRkcq2FGkOsCXCR+2o/IQuqLMdai75sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAtXNqcGkSnob8RmCPzwBlVkZPL6Z3uBnlEk7dzv0zoEMAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAAEIvXAAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAIFZeqoAAAAAAAAAAEAAAAAkc5K9uSAJh3Grr/wm2S3LNl3OEtgRslgshd4jkxu6+4AAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAABPWIAAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAAAuKGgAAAAAAAAAAEAAAAAkc5K9uSAJh3Grr/wm2S3LNl3OEtgRslgshd4jkxu6+4AAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAsxpAcS6M5dE+RQgqqRflcY+NQTB6UB+83oUvnHjPU9IAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAABnesiqAAAAAAAAAAEAAAAAnPXA1s0+/qsD0saYDm3OOP4i244eRFnm0Zoncx1zL+sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAABnesiqAAAAAAAAAAEAAAAAnPXA1s0+/qsD0saYDm3OOP4i244eRFnm0Zoncx1zL+sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAJs4LUUAAAAAAAAAAGEXrs1AAAAQKli297VQldRucMvFo7dC5bm+4ajMlv/a3zl18JIkOSXH4NwplUx0wsQbV0JHBbeHeM4AInlOUqxczu/2pCpXAY=";

static const char* tx_four = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAyAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAACAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAQAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAFVU0QAAAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAAA9ZI2AAAAAAAAAAAeJoetEAAABA9DFFgiaosjqQBD9HZPyVwxpmLzTOFscmzCZBBM/3Y1VCpR+u5VNeDDxLs42XdCgbadqfGBfdI4ypbgw8yT0MDw==";

static const char* tx_five = "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";

void runDeserializationTests()
{
    testDeserialize(tx_one, 1, 1, "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(tx_two, 1, 1, "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(tx_three, 10, 1, "GBIX4B64YECP2WEWYE52Q7O7QMYWADC2VOQQF7MOJ3VWYW4EL25TKIXK");
    testDeserialize(tx_four, 2, 1, "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    // tx_five has 25 ManageSellOffer operations and 2 signatures
    testDeserialize(tx_five, 25, 2, "GBCHVP6PGXGH6ALPV5K5LD56KJDTL7W4XBDQGNF65DPADRH4VOXVDIDG");
}

extern void
runStellarTest (void /* ... */) {
    runAccountTests();
    runSerializationTests();
    runDeserializationTests();
}
