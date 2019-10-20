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
#include "support/BRCrypto.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRBIP39WordsEn.h"
#include "support/BRKey.h"

#include "BRHederaTransaction.h"
#include "BRHederaAccount.h"
#include "BRHederaWallet.h"

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

const char * paper_key_24 = "inmate flip alley wear offer often piece magnet surge toddler submit right radio absent pear floor belt raven price stove replace reduce plate home";
const char * public_key_24 = "b63b3815f453cf697b53b290b1d78e88c725d39bde52c34c79fb5b4c93894673";
const char * paper_key_12 = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
const char * public_key_12 = "ec7554cc83ba25a9b6ca44f491de24881af4faba8805ba518db751d62f675585";
// 0.0.114008 Hedera accountID
const char * paper_key_target = "choose color rich dose toss winter dutch cannon over air cash market";
const char * public_key_target = "372c41776cbdb5cacc7c41ec75b17ad9bd3f242f5c4ab13a1bbeef274d454404";
// 0.0.114009 Hedera accountID

static BRHederaAccount getDefaultAccount()
{
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key_12, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRHederaAddress address;
    address.shard = 0;
    address.realm = 0;
    address.account = 114008;
    hederaAccountSetAddress(account, address);
    return account;
}

static void createNewTransaction() {
    // Create a hedera account
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key_24, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);

    BRKey publicKey = hederaAccountGetPublicKey(account);

    BRHederaAddress source;
    BRHederaAddress target;
    source.shard = 0;
    source.realm = 0;
    source.account = 55;
    target = source;
    target.account = 78;
    BRHederaTransaction transaction = hederaTransactionCreateNew(source, target, 400);

    BRHederaAddress nodeAddress = source;
    nodeAddress.account = 2;
    BRHederaTimeStamp timeStamp;
    timeStamp.seconds = 25;
    timeStamp.nano = 4;
    hederaTransactionSignTransaction(transaction, publicKey, nodeAddress, timeStamp, 100000, seed);
    size_t serializedSize = 0;
    uint8_t * serializedBytes = hederaTransactionSerialize(transaction, &serializedSize);
    if (debug_log) printBytes("Serialized bytes: ", serializedBytes, serializedSize);
    // The following output came from a hedera-sdk-java unit test with the same parameters
    const char * expectedOutput = "1a660a640a20b63b3815f453cf697b53b290b1d78e88c725d39bde52c34c79fb5b4c938946731a401a27eaa0d2f04302ea1722cf111b5f91429c0cba10eca1f7417154779c37c2e487b07a36706a75cbafaf048326a8c199b7832d6668c88585934dcf639ff6bd0d222e0a0a0a0408191004120218371202180218a08d062202087872140a120a070a021837109f060a070a02184e10a006";
    // Convert the expected output to bytes and compare
    uint8_t expected_output_bytes[strlen(expectedOutput)];
    hex2bin(expectedOutput, expected_output_bytes);
    assert(memcmp(expected_output_bytes, serializedBytes, serializedSize) == 0);
}

static void createExistingTransaction()
{
    // Create a hedera account
    BRHederaAccount account = getDefaultAccount();

    BRHederaAddress source;
    BRHederaAddress target;
    source.shard = 0;
    source.realm = 0;
    source.account = 55;
    target = source;
    target.account = 78;
    const char * txId = "0.0.14623-1568420904-460838529";
    BRHederaTransactionHash expectedHash;
    // Create a fake hash for this transaction
    BRSHA256(expectedHash.bytes, paper_key_24, strlen(paper_key_24));
    BRHederaTransaction transaction = hederaTransactionCreate(source, target, 400, txId, expectedHash);

    // Check the values
    BRHederaTransactionHash hash = hederaTransactionGetHash(transaction);
    assert(memcmp(hash.bytes, expectedHash.bytes, 32) == 0);
    BRHederaAddress tempAddress = hederaTransactionGetSource(transaction);
    assert(hederaAddressEqual(source, tempAddress) == 1);
    tempAddress = hederaTransactionGetTarget(transaction);
    assert(hederaAddressEqual(target, tempAddress) == 1);

    BRHederaUnitTinyBar amount = hederaTransactionGetAmount(transaction);
    assert(amount == 400);

    BRHederaTransactionId transactionId = hederaTransactionGetTransactionId(transaction);
    assert(transactionId.address.account == 14623);
    assert(transactionId.timeStamp.seconds == 1568420904);
    assert(transactionId.timeStamp.nano == 460838529);

    hederaTransactionFree(transaction);
    hederaAccountFree(account);
}

static void hederaAccountCheckPublicKey(const char * paper_key, const char * public_key_string)
{
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRHederaAddress address;
    address.shard = 0;
    address.realm = 0;
    address.account = 1000;
    hederaAccountSetAddress(account, address);
    BRKey publicKey = hederaAccountGetPublicKey(account);
    // Validate the public key
    uint8_t expected_public_key[32];
    hex2bin(public_key_string, expected_public_key);
    assert(memcmp(expected_public_key, publicKey.pubKey, 32) == 0);
}

static void accountStringTest(const char * paper_key) {
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRHederaAddress address;
    address.shard = 0;
    address.realm = 0;
    address.account = 114008;
    hederaAccountSetAddress(account, address);
    // Get the length of the account string
    size_t length = hederaAccountGetAddressString(account, NULL, 0);
    assert(length == 11); // 0.0.114008 plus NULL byte
    char addressString[length];
    hederaAccountGetAddressString(account, addressString, length);
    assert(strcmp(addressString, "0.0.114008") == 0);
}

static void addressEqualTest()
{
    BRHederaAddress a1;
    a1.shard = 0;
    a1.realm = 0;
    a1.account = 25;
    BRHederaAddress a2 = a1;
    assert(1 == hederaAddressEqual(a1, a2));
    a1.account = 26;
    assert(0 == hederaAddressEqual(a1, a2));
}

static void createAndDeleteWallet()
{
    BRHederaAccount account = getDefaultAccount();

    BRHederaWallet wallet = hederaWalletCreate(account);

    // Source and target addresses should be the same for Hedera
    BRHederaAddress expectedAddress;
    expectedAddress.shard = 0;
    expectedAddress.realm = 0;
    expectedAddress.account = 114008;

    BRHederaAddress sourceAddress = hederaWalletGetSourceAddress(wallet);
    assert(hederaAddressEqual(sourceAddress, expectedAddress) == 1);

    BRHederaAddress targetAddress = hederaWalletGetTargetAddress(wallet);
    assert(hederaAddressEqual(targetAddress, expectedAddress) == 1);

    hederaAccountFree(account);
    hederaWalletFree(wallet);
}

static void walletBalanceTests()
{
    BRHederaAccount account = getDefaultAccount ();
    BRHederaWallet wallet = hederaWalletCreate (account);
    BRHederaUnitTinyBar expectedBalance = 1000000000;
    hederaWalletSetBalance (wallet, expectedBalance);
    BRHederaUnitTinyBar balance = hederaWalletGetBalance (wallet);
    assert(balance == expectedBalance);
}

static void transaction_tests() {
    createNewTransaction();
    createExistingTransaction();
}

static void account_tests() {
    hederaAccountCheckPublicKey(paper_key_24, public_key_24);
    hederaAccountCheckPublicKey(paper_key_12, public_key_12);
    hederaAccountCheckPublicKey(paper_key_target, public_key_target);

    accountStringTest(paper_key_12);

    addressEqualTest();
}

static void wallet_tests()
{
    createAndDeleteWallet();
    walletBalanceTests();
}

extern void
runHederaTest (void /* ... */) {
    printf("Running hedera unit tests...\n");
    account_tests();
    transaction_tests();
    wallet_tests();
}
