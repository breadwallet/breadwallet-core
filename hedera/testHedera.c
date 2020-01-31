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
#include <stdbool.h>
#include <time.h>
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

static void bin2HexString (uint8_t *input, size_t inputSize, char * output) {
    for (size_t i = 0; i < inputSize; i++) {
        sprintf(&output[i*2], "%02x", input[i]);
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

/*
const char * paper_key_24 = "inmate flip alley wear offer often piece magnet surge toddler submit right radio absent pear floor belt raven price stove replace reduce plate home";
const char * public_key_24 = "b63b3815f453cf697b53b290b1d78e88c725d39bde52c34c79fb5b4c93894673";
const char * paper_key_12 = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
const char * paper_key_patient = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
const char * public_key_12 = "ec7554cc83ba25a9b6ca44f491de24881af4faba8805ba518db751d62f675585";
const char * paper_key_target = "choose color rich dose toss winter dutch cannon over air cash market";
const char * public_key_target = "372c41776cbdb5cacc7c41ec75b17ad9bd3f242f5c4ab13a1bbeef274d454404";
*/

struct account_info {
    const char * name;
    const char * account_string;
    const char * paper_key;
    const char * public_key;
};

struct account_info accounts[] = {
    {"patient", "0.0.114008", "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone",
        "ec7554cc83ba25a9b6ca44f491de24881af4faba8805ba518db751d62f675585"
    } ,
    {"choose", "0.0.114009", "choose color rich dose toss winter dutch cannon over air cash market",
        "372c41776cbdb5cacc7c41ec75b17ad9bd3f242f5c4ab13a1bbeef274d454404" },
    {"node3", "0.0.3", "", ""},
    {"node2", "0.0.2", "", ""},
    {"nodetest", "5412398.75.101101101", "", ""},
    {"inmate", "", "inmate flip alley wear offer often piece magnet surge toddler submit right radio absent pear floor belt raven price stove replace reduce plate home", "b63b3815f453cf697b53b290b1d78e88c725d39bde52c34c79fb5b4c93894673"}
};
size_t num_accounts = sizeof (accounts) / sizeof (struct account_info);

struct account_info find_account (const char * accountName) {
    for (size_t i = 0; i < num_accounts; i++) {
        if (strcmp(accounts[i].name, accountName) == 0)
            return accounts[i];
    }
    // If we get to here just return the first account
    return accounts[0];
}

static int checkAddress (BRHederaAddress address, const char * accountName)
{
    struct account_info accountInfo = find_account (accountName);
    BRHederaAddress accountAddress = hederaAddressCreateFromString(accountInfo.account_string);
    int equal = hederaAddressEqual(address, accountAddress);
    hederaAddressFree (accountAddress);
    return equal;
}

static BRHederaAccount getDefaultAccount()
{
    // There is no account named "default_account" BUT the find function
    // will give me back a valid account anyway.
    struct account_info default_account = find_account("default_account");
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, default_account.paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRHederaAddress address = hederaAddressCreateFromString(default_account.account_string);
    hederaAccountSetAddress(account, address);
    return account;
}

static BRHederaTransaction createSignedTransaction (const char * source, const char * target, const char * node,
                                  int64_t amount, int64_t seconds, int32_t nanos,
                                  int64_t fee)
{
    struct account_info source_account = find_account (source);
    struct account_info target_account = find_account (target);
    struct account_info node_account = find_account (node);

    // Create a hedera account
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, source_account.paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);

    BRHederaAddress sourceAddress = hederaAddressCreateFromString (source_account.account_string);
    BRHederaAddress targetAddress = hederaAddressCreateFromString (target_account.account_string);
    BRHederaAddress nodeAddress = hederaAddressCreateFromString (node_account.account_string);
    BRHederaTimeStamp timeStamp;
    timeStamp.seconds = seconds;
    timeStamp.nano = nanos;
    BRHederaFeeBasis feeBasis;
    feeBasis.costFactor = 1;
    feeBasis.pricePerCostFactor = fee;
    BRHederaTransaction transaction = hederaTransactionCreateNew(sourceAddress, targetAddress, amount,
                                                                 feeBasis, nodeAddress, &timeStamp);

    // Sign the transaction
    BRKey publicKey = hederaAccountGetPublicKey(account);
    hederaTransactionSignTransaction (transaction, publicKey, seed);

    // Cleaup
    hederaAddressFree (sourceAddress);
    hederaAddressFree (targetAddress);
    hederaAddressFree (nodeAddress);
    hederaAccountFree (account);

    return transaction;
}

static void createNewTransaction (const char * source, const char * target, const char * node,
                                   int64_t amount, int64_t seconds, int32_t nanos,
                                   int64_t fee, const char * expectedOutput, bool printOutput)
{
    BRHederaTransaction transaction = createSignedTransaction(source, target, node, amount, seconds, nanos, fee);

    // Get the signed bytes
    size_t serializedSize = 0;
    uint8_t * serializedBytes = hederaTransactionSerialize(transaction, &serializedSize);
    char * transactionOutput = NULL;
    if (printOutput || expectedOutput) {
        transactionOutput = calloc (1, (serializedSize * 2) + 1);
        bin2HexString (serializedBytes, serializedSize, transactionOutput);
        if (expectedOutput) {
            assert (strcmp (expectedOutput, transactionOutput) == 0);
        }
        if (printOutput) {
            printf("Transaction output:\n%s\n", transactionOutput);
        }
        free (transactionOutput);
    }

    // Cleanup
    hederaTransactionFree (transaction);
}

static void transaction_value_test(const char * source, const char * target, const char * node, int64_t amount,
                                   int64_t seconds, int32_t nanos, int64_t fee)
{
    BRHederaTransaction transaction = createSignedTransaction(source, target, node, amount, seconds, nanos, fee);
    // Check the fee and amount
    BRHederaUnitTinyBar txFee = hederaTransactionGetFee (transaction);
    assert (txFee == fee);
    BRHederaUnitTinyBar txAmount = hederaTransactionGetAmount (transaction);
    assert (txAmount == amount);

    // Check the addresses
    BRHederaAddress sourceAddress = hederaTransactionGetSource (transaction);
    BRHederaAddress targetAddress = hederaTransactionGetTarget (transaction);
    assert (1 == checkAddress(sourceAddress, source));
    assert (1 == checkAddress(targetAddress, target));

    hederaAddressFree (sourceAddress);
    hederaAddressFree (targetAddress);
    hederaTransactionFree (transaction);
}

static void createExistingTransaction(const char * sourceUserName, const char *targetUserName, int64_t amount)
{
    struct account_info sourceAccountInfo  = find_account (sourceUserName);
    struct account_info targetAccountInfo = find_account (targetUserName);
    // Create a hedera account
    BRHederaAccount account = getDefaultAccount();

    BRHederaAddress source = hederaAddressCreateFromString (sourceAccountInfo.account_string);
    BRHederaAddress target = hederaAddressCreateFromString (targetAccountInfo.account_string);

    const char * txId = "hedera-mainnet:0.0.14623-1568420904-460838529-0";
    BRHederaTransactionHash expectedHash;
    // Create a fake hash for this transaction
    BRSHA256(expectedHash.bytes, sourceAccountInfo.paper_key, strlen(sourceAccountInfo.paper_key));
    BRHederaUnitTinyBar fee = 500000;
    uint64_t timestamp = 1000;
    uint64_t blockHeight = 1000;
    BRHederaTransaction transaction = hederaTransactionCreate(source, target, amount, fee, txId, expectedHash,
                                                              timestamp, blockHeight);

    // Check the values
    BRHederaTransactionHash hash = hederaTransactionGetHash(transaction);
    assert(memcmp(hash.bytes, expectedHash.bytes, 32) == 0);
    BRHederaAddress address = hederaTransactionGetSource(transaction);
    assert(hederaAddressEqual(source, address) == 1);
    hederaAddressFree(address);

    address = hederaTransactionGetTarget(transaction);
    assert(hederaAddressEqual(target, address) == 1);
    hederaAddressFree(address);

    BRHederaUnitTinyBar txAmount = hederaTransactionGetAmount(transaction);
    assert(amount == txAmount);

    char * transactionId = hederaTransactionGetTransactionId(transaction);
    assert(strcmp(txId, transactionId) == 0);
    free (transactionId);

    hederaTransactionFree(transaction);
    hederaAccountFree(account);
    hederaAddressFree (source);
    hederaAddressFree (target);
}

static void hederaAccountCheckPublicKey(const char * userName)
{
    struct account_info accountInfo = find_account (userName);
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, accountInfo.paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRKey publicKey = hederaAccountGetPublicKey(account);
    // Validate the public key
    uint8_t expected_public_key[32];
    hex2bin(accountInfo.public_key, expected_public_key);
    assert(memcmp(expected_public_key, publicKey.pubKey, 32) == 0);
}

static void hederaAccountCheckSerialize(const char * userName)
{
    struct account_info accountInfo = find_account (userName);
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, accountInfo.paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed(seed);
    BRHederaAddress address = hederaAddressCreateFromString(accountInfo.account_string);
    BRKey key1 = hederaAccountGetPublicKey(account);
    hederaAccountSetAddress(account, address);
    size_t accountByteSize = 0;
    uint8_t * accountBytes = hederaAccountGetSerialization(account, &accountByteSize);
    // Now create a new account
    BRHederaAccount account2 = hederaAccountCreateWithSerialization(accountBytes, accountByteSize);
    BRHederaAddress account2Address = hederaAccountGetAddress(account2);
    assert( 1 == hederaAddressEqual(address, account2Address));

    BRKey key2 = hederaAccountGetPublicKey(account2);
    assert(0 == memcmp(key1.pubKey, key2.pubKey, 32));

    hederaAddressFree(address);
    hederaAddressFree(account2Address);
    hederaAccountFree(account);
    hederaAccountFree(account2);
}

static void accountStringTest(const char * userName) {
    struct account_info accountInfo = find_account (userName);
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, accountInfo.paper_key, NULL); // no passphrase
    BRHederaAccount account = hederaAccountCreateWithSeed (seed);
    BRHederaAddress inAddress = hederaAddressCreateFromString (accountInfo.account_string);
    hederaAccountSetAddress (account, inAddress);

    // Now get the address from the account
    BRHederaAddress outAddress = hederaAccountGetAddress (account);
    assert(hederaAddressEqual(inAddress, outAddress) == 1);

    char * outAddressString = hederaAddressAsString (outAddress);
    assert(strcmp(outAddressString, accountInfo.account_string) == 0);

    // Cleanup
    hederaAddressFree (inAddress);
    hederaAddressFree (outAddress);
    free (outAddressString);
    hederaAccountFree (account);
}

static void addressEqualTests()
{
    BRHederaAddress a1 = hederaAddressCreateFromString("0.0.1000000");
    BRHederaAddress a2 = hederaAddressClone (a1);
    BRHederaAddress a3 = hederaAddressCreateFromString("0.0.1000000");
    assert(1 == hederaAddressEqual(a1, a2));
    assert(1 == hederaAddressEqual(a1, a3));

    // now check no equal
    BRHederaAddress a4 = hederaAddressCreateFromString("0.0.1000001");
    assert(0 == hederaAddressEqual(a1, a4));

    hederaAddressFree (a1);
    hederaAddressFree (a2);
    hederaAddressFree (a3);
    hederaAddressFree (a4);
}

static void addressCloneTests()
{
    BRHederaAddress a1 = hederaAddressCreateFromString("0.0.1000000");
    BRHederaAddress a2 = hederaAddressClone (a1);
    BRHederaAddress a3 = hederaAddressClone (a1);
    BRHederaAddress a4 = hederaAddressClone (a1);
    BRHederaAddress a5 = hederaAddressClone (a2);
    BRHederaAddress a6 = hederaAddressClone (a3);

    assert(1 == hederaAddressEqual(a1, a2));
    assert(1 == hederaAddressEqual(a1, a3));
    assert(1 == hederaAddressEqual(a1, a4));
    assert(1 == hederaAddressEqual(a1, a5));
    assert(1 == hederaAddressEqual(a1, a6));
    assert(1 == hederaAddressEqual(a2, a3));

    // If we indeed have copies then we should not get a double free
    // assert error here.
    hederaAddressFree (a1);
    hederaAddressFree (a2);
    hederaAddressFree (a3);
    hederaAddressFree (a4);
    hederaAddressFree (a5);
    hederaAddressFree (a6);
}

static void addressFeeTests()
{
    BRHederaAddress feeAddress = hederaAddressCreateFromString("__fee__");
    assert (1 == hederaAddressIsFeeAddress(feeAddress));
    char * feeAddressString = hederaAddressAsString (feeAddress);
    assert(0 == strcmp(feeAddressString, "__fee__"));
    free (feeAddressString);
    hederaAddressFree(feeAddress);

    BRHederaAddress address = hederaAddressCreateFromString("0.0.3");
    assert (0 == hederaAddressIsFeeAddress(address));
    hederaAddressFree(address);
}

static void addressValueTests() {
    BRHederaAddress address = hederaAddressCreateFromString("0.0.0");
    assert(0 == hederaAddressGetShard (address));
    assert(0 == hederaAddressGetRealm (address));
    assert(0 == hederaAddressGetAccount (address));
    hederaAddressFree (address);

    // Check a max int64 account number
    address = hederaAddressCreateFromString("0.0.9223372036854775807");
    assert(0 == hederaAddressGetShard (address));
    assert(0 == hederaAddressGetRealm (address));
    assert(9223372036854775807 == hederaAddressGetAccount (address));
    hederaAddressFree (address);

    // Check when all numbers are max int64
    address = hederaAddressCreateFromString("9223372036854775807.9223372036854775807.9223372036854775807");
    assert(9223372036854775807 == hederaAddressGetShard (address));
    assert(9223372036854775807 == hederaAddressGetRealm (address));
    assert(9223372036854775807 == hederaAddressGetAccount (address));
    hederaAddressFree (address);

    // TODO - Check when the number (string) is too long
    //address = hederaAddressCreateFromString("0.0.9223372036854775807999");
    //assert(address == NULL);

    // TODO - Check when the number is the the correct length but is greater than max int64
    //address = hederaAddressCreateFromString("0.0.9993372036854775807");
    //assert(address == NULL);

    // Check when the string is invalid
    address = hederaAddressCreateFromString("0.0");
    assert(address == NULL);

    // Check an empty string
    address = hederaAddressCreateFromString("");
    assert(address == NULL);

    // Check an null - cannot check this since due to assert in function
    // address = hederaAddressCreateFromString(NULL);
    // assert(address == NULL);
}

static void createAndDeleteWallet()
{
    BRHederaAccount account = getDefaultAccount();

    BRHederaWallet wallet = hederaWalletCreate(account);

    // Source and target addresses should be the same for Hedera
    struct account_info defaultAccountInfo = find_account("default_account");
    BRHederaAddress expectedAddress = hederaAddressCreateFromString(defaultAccountInfo.account_string);

    BRHederaAddress sourceAddress = hederaWalletGetSourceAddress(wallet);
    assert(hederaAddressEqual(sourceAddress, expectedAddress) == 1);

    BRHederaAddress targetAddress = hederaWalletGetTargetAddress(wallet);
    assert(hederaAddressEqual(targetAddress, expectedAddress) == 1);

    hederaAccountFree(account);
    hederaWalletFree(wallet);
    hederaAddressFree (expectedAddress);
    hederaAddressFree (sourceAddress);
    hederaAddressFree (targetAddress);
}

static void walletBalanceTests()
{
    BRHederaAccount account = getDefaultAccount ();
    BRHederaWallet wallet = hederaWalletCreate (account);
    BRHederaUnitTinyBar expectedBalance = 1000000000;
    hederaWalletSetBalance (wallet, expectedBalance);
    BRHederaUnitTinyBar balance = hederaWalletGetBalance (wallet);
    assert(balance == expectedBalance);

    hederaAccountFree (account);
    hederaWalletFree (wallet);
}

static void create_real_transactions() {
    // use the function to create sendable transactions to the hedera network
    time_t now;
    now = time(&now);
    printf ("now: %ld\n", now);
    // Send 10,000,000 tiny bars from patient to chose via node3
    createNewTransaction ("patient", "choose", "node3", 10000000, now, 0, 500000, NULL, true);

    createNewTransaction ("choose", "patient", "node3", 50000000, now, 0, 500000, NULL, true);
}

static void create_new_transactions() {
    const char * testOneOutput = "0000000000000000000000000000000000000000000000031a660a640a20ec7554cc83ba25a9b6ca44f491de24881af4faba8805ba518db751d62f6755851a40e9086013e266e779a08a6b5f56efef98a1d9a9a5d3dce2f40dba01b35ea429247872c98e2fe0f6150ba3d82e7b9848a2c95d118d9f8bc66ae285be42d1e94407223b0a0e0a060889f0c6ed05120418d8fa061202180318a0c21e220308b401721c0a1a0a0b0a0418d8fa0610ffd9c4090a0b0a0418d9fa061080dac409";
    // Send 10,000,000 tiny bars to "choose" from "patient" via node3.
    createNewTransaction ("patient", "choose", "node3", 10000000, 1571928073, 0, 500000, testOneOutput, false);

    const char * testTwoOutput = "0000000000000000000000000000000000000000000000031a660a640a20372c41776cbdb5cacc7c41ec75b17ad9bd3f242f5c4ab13a1bbeef274d4544041a40be090d58fb3926c5e3e3f8bd19badca4189a42d7ce336bf4e736738bf3932c8b9a12e79bcab3e94beeca17e2acd027c6baedc8b74d70b63669319927bb39f700223b0a0e0a0608d1f1c6ed05120418d9fa061202180318a0c21e220308b401721c0a1a0a0b0a0418d9fa0610ffc1d72f0a0b0a0418d8fa061080c2d72f";
    // Send 50,000,000 tiny bars to "patient" from "choose" via node3
    createNewTransaction ("choose", "patient", "node3", 50000000, 1571928273, 0, 500000, testTwoOutput, false);
}

static void address_tests() {
    addressEqualTests();
    addressValueTests();
    addressFeeTests();
    addressCloneTests();
}

static void account_tests() {
    hederaAccountCheckPublicKey("inmate");
    hederaAccountCheckPublicKey("patient");
    hederaAccountCheckPublicKey("choose");

    hederaAccountCheckSerialize("patient");

    accountStringTest("patient");
}

static void wallet_tests()
{
    createAndDeleteWallet();
    walletBalanceTests();
}

static void transaction_tests() {
    createExistingTransaction("patient", "choose", 400);
    create_new_transactions();
    transaction_value_test("patient", "choose", "node3", 10000000, 25, 4, 500000);
    //create_real_transactions();
}

static void txIDTests() {
    const char * txID1 = "hedera-mainnet:0.0.14222-1569828647-256912000-0";
    BRHederaTimeStamp ts1 = hederaParseTimeStamp(txID1);
    assert(ts1.seconds == 1569828647);
    assert(ts1.nano == 256912000);
}

extern void
runHederaTest (void /* ... */) {
    printf("Running hedera unit tests...\n");
    address_tests();
    account_tests();
    wallet_tests();
    transaction_tests();
    txIDTests();
}
