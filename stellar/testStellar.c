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
                            uint32_t expectedOpCount,
                            uint32_t expectedSignatureCount,
                            const char* expectedMemoText,
                            const char* accountString)
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
    if (accountString) {
        BRStellarAccountID accountID = stellarTransactionGetAccountID(transaction);
        BRKey key;
        memcpy(key.pubKey, accountID.accountID, 32);
        BRStellarAddress address = createStellarAddressFromPublicKey(&key);
        assert(0 == strcmp(&address.bytes[0], accountString));
    }
    size_t opCount = stellarTransactionGetOperationCount(transaction);
    assert(opCount == expectedOpCount);
    uint32_t sigCount = stellarTransactionGetSignatureCount(transaction);
    assert(sigCount == expectedSignatureCount);

    if (expectedMemoText) {
        BRStellarMemo * memo = stellarTransactionGetMemo(transaction);
        assert(memo);
        assert(0 == strcmp(expectedMemoText, memo->text));
    }
    stellarTransactionFree(transaction);
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


static const char* tx_one = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAGAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABA7SA5lCfGXhKqo44uczRi9kIIOVaAv02ugAIWK8vxVDDPk5zvjIbffBTDOhJpaf4kxnvsar7NWVHhsd+ieIyYCQ==";

static const char* tx_two = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAHAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAeJoetEAAABAlLvA6YjDlERdXd1gU5VYeczu26F+Wgt0VpGsfqdN0kgUx1B7GFdmB2tT2tKM72XLYu7Y2M6+c5QiDueVNP45BQ==";

static const char* tx_three = "AAAAAFF+B9zBBP1YlsE7qH3fgzFgDFqroQL9jk7rbFuEXrs1AAAD6AAIMqMAAALHAAAAAAAAAAEAAAATMTU1OTc3NTY0NDM4NzY2MzMxNwAAAAAKAAAAAAAAAAEAAAAA6Twf5NVbRbK9xRkcq2FGkOsCXCR+2o/IQuqLMdai75sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAtXNqcGkSnob8RmCPzwBlVkZPL6Z3uBnlEk7dzv0zoEMAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAAEIvXAAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAIFZeqoAAAAAAAAAAEAAAAAkc5K9uSAJh3Grr/wm2S3LNl3OEtgRslgshd4jkxu6+4AAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAABPWIAAAAAAAAAAAEAAAAAujyXUtPD0YI01M1C5/c2er0UmMY7KEjbgNapoXfTfaEAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAAAuKGgAAAAAAAAAAEAAAAAkc5K9uSAJh3Grr/wm2S3LNl3OEtgRslgshd4jkxu6+4AAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAADO9ZFKAAAAAAAAAAEAAAAAsxpAcS6M5dE+RQgqqRflcY+NQTB6UB+83oUvnHjPU9IAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAABnesiqAAAAAAAAAAEAAAAAnPXA1s0+/qsD0saYDm3OOP4i244eRFnm0Zoncx1zL+sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAABnesiqAAAAAAAAAAEAAAAAnPXA1s0+/qsD0saYDm3OOP4i244eRFnm0Zoncx1zL+sAAAABQVNUAAAAAAA7RJmpRASHTO3fxjKgzKNxcMOAIuzHdHxKqG/pKFzBeQAAAAJs4LUUAAAAAAAAAAGEXrs1AAAAQKli297VQldRucMvFo7dC5bm+4ajMlv/a3zl18JIkOSXH4NwplUx0wsQbV0JHBbeHeM4AInlOUqxczu/2pCpXAY=";

static const char* tx_four = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAyAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAACAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAQAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAFVU0QAAAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAAA9ZI2AAAAAAAAAAAeJoetEAAABA9DFFgiaosjqQBD9HZPyVwxpmLzTOFscmzCZBBM/3Y1VCpR+u5VNeDDxLs42XdCgbadqfGBfdI4ypbgw8yT0MDw==";

static const char* manage_sell_offer = "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";

static const char * create_account = "AAAAABazwKAoKLArxulrNcFFC77uk62XehKoGtw88Esm/2j1AAAAZAAKSLoAAAAXAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAEAAAAAEH3Rayw4M0iCLoEe96rPFNGYim8AVHJU0z4ebYZW4JwAAAAAAAAAAEwgN7rJa6CnAywT3bO9D+O+l6DWfffoS1hxhBZj1XiLAAAAF0h26AAAAAAAAAAAAib/aPUAAABAnk4Zpl2aLtahfwbkhnLCsBg5TpvNAHzpkk1o/OlcF9cH6SiWHOyOd7NBg8Gz3J5IBBdHPHP9/f9knsV/aOAhA4ZW4JwAAABANEaa9FU68H4dtIcPsXJYk2xjyYKyNauVm4a1eBjQ5R9F85eCHQ5hxmf/TMW6F28Iu/X9dLowjYjz+zNYkWPaCQ==";

static const char * create_account_with_memo = "AAAAAAFOcspucax5xw99HWyyCAZ+FS7Mit+5U1rVJyv4+ZnQAAAAZAAGuQEAAWEVAAAAAQAAAAAAAAAAAAAAAFz5YIQAAAABAAAABVdIQUxFAAAAAAAAAQAAAAAAAAAAAAAAACJVGoisBGLnBXw0Z9q6aY8vGagvvbHf1DtUhefnCOlLAAAAAAtTK4AAAAAAAAAAAfj5mdAAAABAW8usstplNLZ+TuRQbYTvB2JXSDeMKbofxmaRQCNJ5HST0Jm+K8XVjaCZ1N8fwqj9QfIt8lgWOffdMPBH/fH2Cg==";

static const char * path_payment = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAIAAAAAAAAAAlQL5AAAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAADuaygAAAAABAAAAAVVTRAAAAAAAJA/+t89BcYGwsJMgNfi8CGsE0WwYsduMYp8RBeJoetEAAAAAAAAAAeJoetEAAABArgtWbZye1KhXNKvWQ9Y+sTbYA5mFL1jIUez0oKWPdtiqhILvEAtrxL6SwWOzF2Z0w8xccu0DQlfYKys3a9bjDA==";
static const char * passive_sell_offer = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAQAAAABVVNEAAAAAAAkD/63z0FxgbCwkyA1+LwIawTRbBix24xinxEF4mh60QAAAAFDRE4AAAAAABazwKAoKLArxulrNcFFC77uk62XehKoGtw88Esm/2j1AAAAAD2KsyAAAACHAAAAZAAAAAAAAAAB4mh60QAAAECDKVlOkWGD88JNJ4U9wJgwzFT3CfqT5eUQCAvVJCVp4ZdwyDZ0aE/0JF3sUYe1WgVAg2AtntkeY8KXNXy7iGcN";
static const char * payment_and_options = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAyAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAACAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAABQAAAAAAAAABAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAtmZWQubmV0d29yawAAAAABAAAAABazwKAoKLArxulrNcFFC77uk62XehKoGtw88Esm/2j1AAAAAQAAAAAAAAAB4mh60QAAAECZIoN8cWovhgMTw/DIapIj/biEfImB6tIywxxHfBL0bxeePPR+C6mI/3LttlN+Tjhf71fMvqU9CxXR7f3wzk8D";

static const char * all_operations = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAACvAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAAHAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAAAQAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAFVU0QAAAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAAA9ZI2AAAAAAAAAAAgAAAAAAAAACVAvkAAAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAAAAAAAO5rKAAAAAAEAAAABVVNEAAAAAAAkD/63z0FxgbCwkyA1+LwIawTRbBix24xinxEF4mh60QAAAAAAAAAEAAAAAVVTRAAAAAAAJA/+t89BcYGwsJMgNfi8CGsE0WwYsduMYp8RBeJoetEAAAABQ0ROAAAAAAAWs8CgKCiwK8bpazXBRQu+7pOtl3oSqBrcPPBLJv9o9QAAAAA9irMgAAAAhwAAAGQAAAAAAAAABQAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAABAAAAAQAAAAEAAAACAAAAAQAAAAMAAAABAAAABAAAAAEAAAAFAAAAAQAAAAYAAAABAAAAC2ZlZC5uZXR3b3JrAAAAAAEAAAAAFrPAoCgosCvG6Ws1wUULvu6TrZd6Eqga3DzwSyb/aPUAAAABAAAAAAAAAAYAAAAAAAAAAHc1lAAAAAAAAAAABwAAAABVYvNEtkcUSLe26+tbrpwc7Mkw7yiGi+K7eLt0KDHnEAAAAAFVU0QAAAAAAQAAAAAAAAAB4mh60QAAAEBgh3Y4HxZfjXS1YbXh+3ZrjrJaVNhiAlQobo4LeOsIx9SlpZfdKE/g0kaBq/OFjCUSjbSgCCvZ4AOU68o59gEG";

static const char * set_options_one = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAUAAAAAAAAAAQAAAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAALZmVkLm5ldHdvcmsAAAAAAQAAAAAWs8CgKCiwK8bpazXBRQu+7pOtl3oSqBrcPPBLJv9o9QAAAAEAAAAAAAAAAeJoetEAAABALqu9TWI9WDIY3fkMW30k0gFHE24hPweoW0Yzy+7QXdiSTPV16EZkVopcjjWJGHa6Xk3HDjGGqAAXntcHmdRNAQ==";
static const char * set_options_all = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAZAAHHCYAAAAIAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAABAAAAAAAAAAUAAAABAAAAAFVi80S2RxRIt7br61uunBzsyTDvKIaL4rt4u3QoMecQAAAAAQAAAAEAAAABAAAAAgAAAAEAAAADAAAAAQAAAAQAAAABAAAABQAAAAEAAAAGAAAAAQAAAAtmZWQubmV0d29yawAAAAABAAAAABazwKAoKLArxulrNcFFC77uk62XehKoGtw88Esm/2j1AAAAAQAAAAAAAAAB4mh60QAAAED5ctKVRkz/OvdKBlHxiGNGJ5xZ+3l4dpaYYmUI3nsYemKWHufcFTbRObecjqbpfm0zC+CQW/zR0rAwmO0GEA8G";

void runDeserializationTests()
{
    testDeserialize(tx_one, 1, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(tx_two, 1, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(tx_three, 10, 1, NULL,
                    "GBIX4B64YECP2WEWYE52Q7O7QMYWADC2VOQQF7MOJ3VWYW4EL25TKIXK");
    testDeserialize(tx_four, 2, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    // tx_five has 25 ManageSellOffer operations and 2 signatures
    testDeserialize(manage_sell_offer, 25, 2, NULL,
                    "GBCHVP6PGXGH6ALPV5K5LD56KJDTL7W4XBDQGNF65DPADRH4VOXVDIDG");
    testDeserialize(create_account, 1, 2, NULL,
                    "GALLHQFAFAULAK6G5FVTLQKFBO7O5E5NS55BFKA23Q6PASZG75UPKANL");
    testDeserialize(create_account_with_memo, 1, 1, "WHALE",
                    "GAAU44WKNZY2Y6OHB56R23FSBADH4FJOZSFN7OKTLLKSOK7Y7GM5AT7Y");
    testDeserialize(path_payment, 1, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(passive_sell_offer, 1, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");
    testDeserialize(payment_and_options, 2, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");

    testDeserialize(all_operations, 7, 1, NULL,
                    "GASA77VXZ5AXDANQWCJSANPYXQEGWBGRNQMLDW4MMKPRCBPCNB5NC77I");

    // For the SetOptions test we want to ensure that we parsed the correct
    // settings values from the operation since there are all optional
    // For this test we have set clearFlags, home domain, and Signer
    uint8_t expectedSettingsOne[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01};
    testDeserializeSetOptions(set_options_one, 1, 1, expectedSettingsOne);
    uint8_t expectedSettingsAll[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    testDeserializeSetOptions(set_options_all, 1, 1, expectedSettingsAll);
}

static void deserializeTxResponse(const char * response_xdr, size_t expectedOpCount, int32_t expectedStatus)
{
    BRStellarTransaction transaction = stellarTransactionCreateFromBytes(NULL, 0);
    assert(transaction);
    BRStellarTransactionResult result = stellarTransactionGetResult(transaction, response_xdr);
    assert(expectedStatus == result.resultCode);
    size_t opCount = stellarTransactionGetOperationCount(transaction);
    assert(opCount == expectedOpCount);
    stellarTransactionFree(transaction);
}

static const char * response_payments = "AAAAAAAAAMgAAAAAAAAAAgAAAAAAAAABAAAAAAAAAAAAAAABAAAAAAAAAAA=";
static const char * bad_sequence = "AAAAAAAAAAD////7AAAAAA==";

static void runResultDeserializationTests()
{
    deserializeTxResponse(response_payments, 2, 0);
    deserializeTxResponse(bad_sequence, 0, -5);
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
    // So whateve is needed with there serialized/signed bytes to submit the transaction
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
