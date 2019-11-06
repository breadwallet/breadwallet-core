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
#include "support/BRArray.h"
#include "BRStellar.h"
#include "BRCrypto.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "BRKey.h"
#include "utils/b64.h"
#include "BRStellarSerialize.h"
#include "BRStellarTransaction.h"
#include "BRStellarAccountUtils.h"
#include "testStellarInput.h"

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

static void printBytes(const char* message, uint8_t * bytes, size_t byteSize)
{
    if (message) printf("%s\n", message);
    for(int i = 0; i < byteSize; i++) {
        if (i >= 0 && i % 8 == 0) printf("\n");
        printf("%02X ", bytes[i]);
    }
    printf("\n");
}

static BRStellarAccount createTestAccount(const char* paper_key,
                              const char* public_key_string, const char* expected_address)
{
    BRStellarAccount account = stellarAccountCreate(paper_key);

    if (public_key_string) {
        uint8_t expected_public_key[32];
        hex2bin(public_key_string, expected_public_key);
        BRKey key = stellarAccountGetPublicKey(account);
        if (debug_log) printBytes("PublicKey:", key.pubKey, 32);
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

    account = createTestAccount("release pudding vault own maximum correct ramp cactus always cradle split space",
                                NULL, "GCWRMSOP3RKTOORIW4FRQQVS6HKPEA4LC4QAFV5KLBIH3FYCG3DNKUZ7");

    stellarAccountFree(account);

    // Account "Ted"
    account = createTestAccount("brave rival swap wrestle gorilla diet lounge farm tennis capital ecology design",
                                NULL, "GDSTAICFVBHMGZ4HI6YEKZSGDR7QGEM4PPREYW2JV3XW7STVM7L5EDYZ");
    stellarAccountFree(account);
}

static void serializeMinimum()
{
    BRStellarAccount account = stellarAccountCreate("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    BRStellarAccountID sourceAddress = stellarAccountGetAccountID(account);
    const char * targetAddress = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    BRStellarAccountID destination = stellerAccountCreateStellarAccountID(targetAddress);

    BRStellarMemo memo;
    memo.memoType = 1;
    strcpy(memo.text, "Buy yourself a beer!");

    BRArrayOf(BRStellarOperation) operations;
    array_new(operations, 2);
    array_add(operations, stellarOperationCreatePayment(&destination,
                                                        stellarAssetCreateAsset("XML", NULL), 10.5));
    array_add(operations, stellarOperationCreatePayment(&destination,
                                                        stellarAssetCreateAsset("USD", &sourceAddress), 25.75));

    uint8_t *buffer = NULL;
    size_t length = stellarSerializeTransaction(&sourceAddress, 200, 2001274371309571, NULL, 0,
                                &memo, operations, 0, NULL, 0, &buffer);
    if (debug_log) printBytes("serialized bytes:", buffer, length);
    free(buffer);
}

static void serializeAndSign()
{
    const char * targetAddress = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    BRStellarAccountID destination = stellerAccountCreateStellarAccountID(targetAddress);

    BRStellarAccount account = stellarAccountCreate("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    stellarAccountSetSequence(account, 2001274371309576);
    stellarAccountSetNetworkType(account, STELLAR_NETWORK_TESTNET);
    BRStellarAccountID accountID = stellarAccountGetAccountID(account);

    BRStellarMemo memo;
    memo.memoType = 1;
    strcpy(memo.text, "Buy yourself a beer!");

    // Add the single operation to the array
    BRArrayOf(BRStellarOperation) operations;
    array_new(operations, 1);
    array_add(operations, stellarOperationCreatePayment(&destination,
                                                        stellarAssetCreateAsset("XML", NULL), 10.5));

    uint32_t fee = 100 * (uint32_t)array_count(operations);
    BRStellarTransaction transaction = stellarTransactionCreate(&accountID, fee, NULL, 0, &memo, operations);
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
        printBytes("sBytes:", sBytes, sSize);
    }
    // Compare with what we are expecting
    const char* expected_b64 = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABAzBQpbrqpbfFozHnwpIATkErUPcb5xesMeFClf5dyd4X0kBw3c6gZUVTtHh3iCZ6eUAEge/lCft6NfXzsHy1HBQ==";
    assert(0 == memcmp(encoded, expected_b64, strlen(expected_b64)));
    assert(strlen(encoded) == strlen(expected_b64));
    free(encoded);

    stellarTransactionFree(transaction);
}

void runSerializationTests()
{
    serializeMinimum();
    serializeAndSign();
}

static void testDeserialize(uint32_t test,
                            const char * envelope_xdr, const char* result_xdr, const char* accountString,
                            int32_t opType,
                            uint32_t expectedOpCount,
                            uint32_t expectedSignatureCount,
                            int32_t expectedStatus,
                            uint32_t expectedResultOpCount
                            )
{
    // Deseralize the actual transaction if applicable
    if (envelope_xdr) {
        // Turn the base64 into bytes
        size_t byteSize = 0;
        uint8_t * bytes = b64_decode_ex(envelope_xdr, strlen(envelope_xdr), &byteSize);
        if (debug_log) {
            for(int i = 0; i < byteSize; i++) {
                if (i >= 0 && i % 8 == 0) printf("\n");
                printf("%02X ", bytes[i]);
            }
            printf("\n");
        }

        BRStellarTransaction transaction = stellarTransactionCreateFromBytes(&bytes[0], byteSize);
        assert(transaction);
        if (accountString) {
            BRStellarAccountID accountID = stellarTransactionGetAccountID(transaction);
            BRKey key;
            memcpy(key.pubKey, accountID.accountID, 32);
            BRStellarAddress address = createStellarAddressFromPublicKey(&key);
            if (0 != strcmp(&address.bytes[0], accountString)) printf("Address: %s\n", address.bytes);
            assert(0 == strcmp(&address.bytes[0], accountString));
        }
        size_t opCount = stellarTransactionGetOperationCount(transaction);
        assert(opCount == expectedOpCount);
        uint32_t sigCount = stellarTransactionGetSignatureCount(transaction);
        assert(sigCount == expectedSignatureCount);

        if (opType >= 0) {
            // There should be at least 1 operation that matches our expectations
            bool found = false;
            for (int i = 0; i < opCount; i++) {
                BRStellarOperation *op = stellarTransactionGetOperation(transaction, i);
                if (op->type == opType) {
                    found = true;
                }
            }
            assert(found);
        }
        stellarTransactionFree(transaction);
    }

    // If we get this far then go ahead and parse the result
    if (result_xdr) {
        BRStellarTransaction resultTransaction = stellarTransactionCreateFromBytes(NULL, 0);
        BRStellarTransactionResult result = stellarTransactionGetResult(resultTransaction, result_xdr);
        assert(expectedStatus == result.resultCode);
        size_t opCount = stellarTransactionGetOperationCount(resultTransaction);
        assert(opCount == expectedResultOpCount);
        stellarTransactionFree(resultTransaction);
    }
}

static void testDeserialize2(const char * testNumber, const char * testType, const char * numOperations, const char * numSignatures,
                             const char * envelop_xdr, const char * result_xdr, const char * account,
                             const char * responseCode, const char * expectedResultOpCount)
{
    if (debug_log) printf("Running deserialize test %s\n", testNumber);
    uint32_t test = atoi(testNumber);
    uint32_t opType = atoi(testType);
    uint32_t expectedOpCount = atoi(numOperations);
    uint32_t expectedSignatureCount = atoi(numSignatures);
    int32_t resCode = atoi(responseCode);
    uint32_t resOpCount = atoi(expectedResultOpCount);
    testDeserialize(test, envelop_xdr, result_xdr, account, opType, expectedOpCount, expectedSignatureCount, resCode, resOpCount);
}

static void testDeserializeSetOptions(const char * input,
                            uint32_t expectedOpCount,
                            uint32_t expectedSignatureCount,
                            uint8_t * expectedSettings)
{
    // Turn the base64 into bytes
    size_t byteSize = 0;
    uint8_t * bytes = b64_decode_ex(input, strlen(input), &byteSize);
    if (debug_log) {
        for(int i = 0; i < byteSize; i++) {
            if (i >= 0 && i % 8 == 0) printf("\n");
            printf("%02X ", bytes[i]);
        }
        printf("\n");
    }

    BRStellarTransaction transaction = stellarTransactionCreateFromBytes(&bytes[0], byteSize);
    assert(transaction);

    // If we get the number of operations and signature correct then we
    // can assume we have parse the bytes properly
    size_t opCount = stellarTransactionGetOperationCount(transaction);
    assert(opCount == expectedOpCount);
    uint32_t sigCount = stellarTransactionGetSignatureCount(transaction);
    assert(sigCount == expectedSignatureCount);
    
    for (int i = 0; i < expectedOpCount; i++) {
        BRStellarOperation *op = stellarTransactionGetOperation(transaction, i);
        if (op->type == 5) // settings
        {
            assert(0 == memcmp(op->operation.options.settings, expectedSettings, 9));
            if (debug_log) {
                printf("Settings object: ");
                size_t numSettings = sizeof(op->operation.options.settings) / sizeof(uint8_t);
                for(int i = 0; i < numSettings; i++) {
                    printf("%02X ", op->operation.options.settings[i]);
                }
                printf("\n");
            }
        }
    }

    stellarTransactionFree(transaction);
}

void runDeserializationTests()
{
    size_t numTests = sizeof(test_input)/sizeof(char*);
    for (int i = 0; i < numTests; i += 7) {
        testDeserialize2(test_input[i], test_input[i+1], test_input[i+2], test_input[i+3],
                         test_input[i+4], test_input[i+5], test_input[i+6], "0", test_input[i+2]);
    }

    // For the SetOptions test we want to ensure that we parsed the correct
    // settings values from the operation since there are all optional
    // For this test we have set clearFlags, home domain, and Signer
    uint8_t expectedSettingsOne[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
    static const char * set_options_one = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAUAAAAAAAAAAQAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAALZmVkLm5ldHdvcmsAAAAAAQAAAAAWs8CgKCiwK8bpazXBRQu+7pOtl3oSqBrcPPBLJv9o9QAAAAEAAAAAAAAAAeJoetEAAABALqu9TWI9WDIY3fkMW30k0gFHE24hPweoW0Yzy+7QXdiSTPV16EZkVopcjjWJGHa6Xk3HDjGGqAAXntcHmdRNAQ==";
    testDeserializeSetOptions(set_options_one, 1, 1, expectedSettingsOne);
    static const char * set_options_all = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAUAAAABAAAAAFVi80S2RxRIt7br61uunBzsyTDvKIaL4rt4u3QoMecQAAAAAQAAAAEAAAABAAAAAgAAAAEAAAADAAAAAQAAAAQAAAABAAAABQAAAAEAAAAGAAAAAQAAAAtmZWQubmV0d29yawAAAAABAAAAABazwKAoKLArxulrNcFFC77uk62XehKoGtw88Esm/2j1AAAAAQAAAAAAAAAB4mh60QAAAED5ctKVRkz/OvdKBlHxiGNGJ5xZ+3l4dpaYYmUI3nsYemKWHufcFTbRObecjqbpfm0zC+CQW/zR0rAwmO0GEA8G";
    uint8_t expectedSettingsAll[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    testDeserializeSetOptions(set_options_all, 1, 1, expectedSettingsAll);
}

static const char * bad_sequence = "AAAAAAAAAAD////7AAAAAA==";
static const char * account_merge_result = "AAAAAAAAAGQAAAAAAAAAAQAAAAAAAAAIAAAAAAAAABdIduecAAAAAA==";
static const char * bump_seq_number_result = "AAAAAAAAAGQAAAAAAAAAAQAAAAAAAAALAAAAAAAAAAA=";
static const char * manage_data_result = "AAAAAAAAAGQAAAAAAAAAAQAAAAAAAAAKAAAAAAAAAAA=";
static const char * manage_buy_offer_result = "AAAAAAAAAGQAAAAAAAAAAQAAAAAAAAAMAAAAAAAAAAAAAAAAAAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAAADRotIAAAAAAAAAAVVTRAAAAAAAJA/+t89BcYGwsJMgNfi8CGsE0WwYsduMYp8RBeJoetEAAAAABSDkQgAAAAwAAAABAAAAAAAAAAAAAAAA";

uint8_t inflation_input[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, // 64-bit fee
    0x00, 0x00, 0x00, 0x00, // 4-byte status code
    0x00, 0x00, 0x00, 0x01, // array size
    0x00, 0x00, 0x00, 0x00, // Element 1 - opInner
    0x00, 0x00, 0x00, 0x09, // operation type - 9 is inflation
    0x00, 0x00, 0x00, 0x00, // operation status, 0 = SUCCESS
    0x00, 0x00, 0x00, 0x02, // number of inflation payouts
    0x00, 0x00, 0x00, 0x00, // account ID type, 0 - ed25519
    0x24, 0x0F, 0xFE, 0xB7, 0xCF, 0x41, 0x71, 0x81, // AccountID
    0xB0, 0xB0, 0x93, 0x20, 0x35, 0xF8, 0xBC, 0x08,
    0x6B, 0x04, 0xD1, 0x6C, 0x18, 0xB1, 0xDB, 0x8C,
    0x62, 0x9F, 0x11, 0x05, 0xE2, 0x68, 0x7A, 0xD1,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x96, 0x90, // amount
    0x00, 0x00, 0x00, 0x00, // account ID type, 0 - ed25519
    0x24, 0x0F, 0xFE, 0xB7, 0xCF, 0x41, 0x71, 0x81, // AccountID
    0xB0, 0xB0, 0x93, 0x20, 0x35, 0xF8, 0xBC, 0x08,
    0x6B, 0x04, 0xD1, 0x6C, 0x18, 0xB1, 0xDB, 0x8C,
    0x62, 0x9F, 0x11, 0x05, 0xE2, 0x68, 0x7A, 0xD1,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x98, 0x96, 0x90, // amount
    0x00, 0x00, 0x00, 0x00 // Version
};

static void runResultDeserializationTests()
{
    // static void testDeserialize2(char * testNumber, char * testType, char * numOperations, char * numSignatures,
    // char * envelop_xdr, char * result_xdr, char * account)
    // NOTE: parameters 2 - 5 are not used unless there is a envelope_xdr, but NULLs are not allowed
    // Once we fine true request-responses for the following types we can remove this code
    // Real transactions can be found here: https://dashboard.stellar.org/
    testDeserialize2("10000", "1", "0", "1", NULL, bad_sequence, NULL, "-5", "0");
    testDeserialize2("10001", "1", "0", "1", NULL, account_merge_result, NULL, "0", "1");
    testDeserialize2("10002", "1", "0", "1", NULL, bump_seq_number_result, NULL, "0", "1");
    testDeserialize2("10003", "1", "0", "1", NULL, manage_data_result, NULL, "0", "1");
    testDeserialize2("10004", "1", "0", "1", NULL, account_merge_result, NULL, "0", "1");
    testDeserialize2("10005", "1", "0", "1", NULL, manage_buy_offer_result, NULL, "0", "1");

    // The is a special case - so far we have no way of finding a real inflation result - so
    char * result_xdr = b64_encode(inflation_input, sizeof(inflation_input));
    testDeserialize2("20000", "9", "1", "1", NULL, result_xdr, NULL, "0", "1");
}

static void createDeleteWalletTest(const char* paperKey, const char* accountKey, const char* accountAddress)
{
    BRStellarAccount account = createTestAccount(paperKey, accountKey, accountAddress);
    
    BRStellarWallet wallet = stellarWalletCreate(account);
    BRStellarAmount startAmount = 1250.7321;
    stellarWalletSetBalance(wallet, startAmount);
    BRStellarAmount amount = stellarWalletGetBalance(wallet);
    assert(startAmount == amount);
    
    BRStellarAddress address = stellarWalletGetSourceAddress(wallet);
    assert(0 == memcmp(address.bytes, accountAddress, strlen(accountAddress)));
    BRStellarAddress targetAddress = stellarWalletGetTargetAddress(wallet);
    assert(0 == memcmp(targetAddress.bytes, accountAddress, strlen(accountAddress)));
    stellarWalletFree(wallet);
    stellarAccountFree(account);
}

static void runWalletTests()
{
    createDeleteWalletTest("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy",
                           "240FFEB7CF417181B0B0932035F8BC086B04D16C18B1DB8C629F1105E2687AD1",
                           "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
}

static void runExampleCode()
{
    // Create an account
    BRStellarAccount account = stellarAccountCreate("off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    stellarAccountSetNetworkType(account, STELLAR_NETWORK_TESTNET);
    BRStellarAccountID accountID = stellarAccountGetAccountID(account);

    // Create a Transaction - whith a single operation
    const char * targetAddress = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    BRStellarAccountID destination = stellerAccountCreateStellarAccountID(targetAddress);
    
    BRArrayOf(BRStellarOperation) operations;
    array_new(operations, 1);
    array_add(operations, stellarOperationCreatePayment(&destination, stellarAssetCreateAsset("XML", NULL), 10.5));

    BRStellarMemo memo;
    memo.memoType = 1;
    strcpy(memo.text, "Buy yourself a beer!");

    uint32_t fee = 100 * (uint32_t)array_count(operations);
    BRStellarTransaction transaction = stellarTransactionCreate(&accountID, fee, NULL, 0, &memo, operations);

    // Now serialize and sign
    stellarAccountSetSequence(account, 2001274371309582);
    BRStellarSerializedTransaction s = stellarAccountSignTransaction(account, transaction,
                                  "off enjoy fatal deliver team nothing auto canvas oak brass fashion happy");
    assert(s);
    // Do whatever is needed with there serialized/signed bytes to submit the transaction
    // i.e. call the following functions:
    // size_t    stellarGetSerializedSize(BRStellarSerializedTransaction s)
    // uint8_t * stellarGetSerializedBytes(BRStellarSerializedTransaction s)

    // Get the hash of the transaction
    BRStellarTransactionHash hash = stellarTransactionGetHash(transaction);

    // This was a real transaction that was sent to the stellar testnet - this was the hash that
    // was returned (using the py_stellar_core Python library).
    const char * hashString = "8ff072db8d7fd38c1230321d94dddb0335365af5bdce09fa9254fe18b90e80e3";
    uint8_t expected_hash[32];
    hex2bin(hashString, expected_hash);
    assert(0 == memcmp(hash.bytes, expected_hash, 32));

    // Now let's parse the result_xdr - again this was the actual string returned from testnet
    const char * result_xdr = "AAAAAAAAAGQAAAAAAAAAAQAAAAAAAAABAAAAAAAAAAA=";
    BRStellarTransactionResult result = stellarTransactionGetResult(transaction, result_xdr);
    assert(ST_TX_SUCCESS == result.resultCode);
    size_t opCount = stellarTransactionGetOperationCount(transaction);
    assert(opCount == 1);

    // Now cleanup
    stellarAccountFree(account);
    stellarTransactionFree(transaction);
}

extern void
runStellarTest (void /* ... */) {
    runAccountTests();
    runSerializationTests();
    runDeserializationTests();
    runResultDeserializationTests();
    runWalletTests();

    runExampleCode();
}
