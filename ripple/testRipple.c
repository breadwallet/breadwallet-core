//
//  BRRipple.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "BRRipple.h"
#include "BRRippleBase58.h"
#include "BRCrypto.h"
#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39WordsEn.h"
#include "BRKey.h"
#include "testRippleTxList1.h"
#include "testRippleTxList2.h"

int debug_log = 0;

uint8_t char2int(char input)
{
    if(input >= '0' && input <= '9')
        return input - '0';
    if(input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if(input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0;
}

void hex2bin(const char* src, uint8_t * target)
{
    while(*src && src[1])
    {
        *(target++) = (char2int(src[0]) << 4) | (char2int(src[1]) & 0xff);
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

static BRRippleAccount createTestRippleAccount(const char* paper_key,
                                           const char* expected_account_address)
{
    BRRippleAccount account = rippleAccountCreate(paper_key);
    
    // Get the string ripple address
    if (expected_account_address) {
        char rippleAddress[36];
        rippleAccountGetAddressString(account, rippleAddress, 36);
        assert(0 == strcmp(expected_account_address, rippleAddress));
    }
    return account;
}

static void
testRippleTransaction (void /* ... */) {
    BRRippleTransaction transaction;

    // Here are the 20 byte addresses
    uint8_t sourceBytes[] = { 0xB5, 0xF7, 0x62, 0x79, 0x8A, 0x53, 0xD5,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    uint8_t destBytes[] = { 0xB6, 0xF8, 0x63, 0x80, 0x8B, 0x54, 0xD6,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    memcpy(sourceAddress.bytes, sourceBytes, 20);
    memcpy(targetAddress.bytes, destBytes, 20);
    
    // Create an account so we can get a public key
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRRippleAccount account = rippleAccountCreate(paper_key);
    transaction = rippleTransactionCreate(sourceAddress, targetAddress, 1000000, 12);
    assert(transaction);

    rippleAccountFree(account);
    rippleTransactionFree(transaction);
}

static void
testRippleTransactionGetters (void /* ... */) {
    BRRippleTransaction transaction;

    // Here are the 20 byte addresses
    uint8_t sourceBytes[] = { 0xB5, 0xF7, 0x62, 0x79, 0x8A, 0x53, 0xD5,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    uint8_t destBytes[] = { 0xB6, 0xF8, 0x63, 0x80, 0x8B, 0x54, 0xD6,
        0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    memcpy(sourceAddress.bytes, sourceBytes, 20);
    memcpy(targetAddress.bytes, destBytes, 20);
    
    // Create an account so we can get a public key
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRRippleAccount account = rippleAccountCreate(paper_key);
    transaction = rippleTransactionCreate(sourceAddress, targetAddress, 1000000, 10);
    assert(transaction);

    uint64_t fee = rippleTransactionGetFee(transaction);
    assert(fee == 0); // before serialization
    
    uint64_t amount = rippleTransactionGetAmount(transaction);
    assert(amount == 1000000);

    // Get the raw amount object
    BRRippleAmount amountRaw = rippleTransactionGetAmountRaw(transaction, RIPPLE_AMOUNT_TYPE_AMOUNT);
    assert(0 == amountRaw.currencyType);
    assert(1000000 == amountRaw.amount.u64Amount);

    uint32_t sequence = rippleTransactionGetSequence(transaction);
    // Since we don't add the sequence until later - it should be 0
    assert(0 == sequence);

    BRRippleAddress source = rippleTransactionGetSource(transaction);
    assert(0 == memcmp(source.bytes, sourceAddress.bytes, 20));

    BRRippleAddress target = rippleTransactionGetTarget(transaction);
    assert(0 == memcmp(target.bytes, targetAddress.bytes, 20));
    
    BRRippleFeeBasis feeBasis = rippleTransactionGetFeeBasis(transaction);
    assert(feeBasis == 10);

    rippleAccountFree(account);
    rippleTransactionFree(transaction);
}

static void
testCreateRippleAccountWithPaperKey (void /* ... */) {
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    // The above set of words should produce the following Ripple account address
    // string: r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a
    // raw bytes - EF FC 27 52 B5 C9 DA 22 88 C5 D0 1F 30 4E C8 29 51 E3 7C A2
    uint8_t expected_bytes[] = { 0xEF, 0xFC, 0x27, 0x52, 0xB5, 0xC9, 0xDA, 0x22, 0x88, 0xC5,
        0xD0, 0x1F, 0x30, 0x4E, 0xC8, 0x29, 0x51, 0xE3, 0x7C, 0xA2 };
    const char* expected_ripple_address = "r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a";

    // Create the account using the paper key
    BRRippleAccount account = rippleAccountCreate(paper_key);
    assert(account);

    // Get the 20 bytes that were created for the account
    BRRippleAddress address = rippleAccountGetAddress(account);
    assert(0 == memcmp(address.bytes, expected_bytes, 20));

    // Get the ripple address string and compare
    char rippleAddress[36];
    rippleAccountGetAddressString(account, rippleAddress, 36);
    assert(0 == strcmp(rippleAddress, expected_ripple_address));

    rippleAccountFree(account);
}

static void
testCreateRippleAccountWithSeed (void /* ... */) {
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    // The above set of words should produce the following Ripple account address
    // string: r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a
    const char* expected_accountid_string = "r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a";

    // Use the above paper key to create a seed value
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key, NULL); // no passphrase

    // If we pass the expected_accountid_string to this function it will validate for us
    BRRippleAccount account = rippleAccountCreateWithSeed(seed);
    assert(account);

    // Get the ripple address string and compare
    char rippleAddress[36];
    rippleAccountGetAddressString(account, rippleAddress, 36);
    assert(0 == strcmp(rippleAddress, expected_accountid_string));

    rippleAccountFree(account);
}

static void
testCreateRippleAccountWithKey (void /* ... */) {
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    // The above set of words should produce the following Ripple account address
    // string: r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a
    const char* expected_accountid_string = "r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a";

    // Use the above paper key to create a seed value
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key, NULL); // no passphrase

    // Create the private key from the seed
    BRKey privateKey;
    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath(&privateKey, &seed, sizeof(UInt512), 5,
                       44 | BIP32_HARD,          // purpose  : BIP-44
                       144 | BIP32_HARD,        // coin_type: Ripple
                       0 | BIP32_HARD,          // account  : <n/a>
                       0,                        // change   : not change
                       0);                   // index    :

    privateKey.compressed = 0;

    // If we pass the expected_accountid_string to this function it will validate for us
    BRRippleAccount account = rippleAccountCreateWithKey(privateKey);
    assert(account);

    // Get the 20 bytes that were created for the account
    char rippleAddress[36];
    rippleAccountGetAddressString(account, rippleAddress, 36);
    assert(0 == strcmp(rippleAddress, expected_accountid_string));

    rippleAccountFree(account);
}

static void
testCreateRippleAccountWithSerializedBytes (void /* ... */) {
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    // The above set of words should produce the following Ripple account address
    // string: r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a
    const char* expected_accountid_string = "r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a";
    
    // Use the above paper key to create a seed value
    UInt512 seed = UINT512_ZERO;
    BRBIP39DeriveKey(seed.u8, paper_key, NULL); // no passphrase
    
    // Create the private key from the seed
    BRKey privateKey;
    // The BIP32 privateKey for m/44'/60'/0'/0/index
    BRBIP32PrivKeyPath(&privateKey, &seed, sizeof(UInt512), 5,
                       44 | BIP32_HARD,          // purpose  : BIP-44
                       144 | BIP32_HARD,        // coin_type: Ripple
                       0 | BIP32_HARD,          // account  : <n/a>
                       0,                        // change   : not change
                       0);                   // index    :
    
    privateKey.compressed = 0;
    
    // If we pass the expected_accountid_string to this function it will validate for us
    BRRippleAccount account = rippleAccountCreateWithKey(privateKey);
    assert(account);
    
    // Get the 20 bytes that were created for the account
    char rippleAddress[36];
    rippleAccountGetAddressString(account, rippleAddress, 36);
    assert(0 == strcmp(rippleAddress, expected_accountid_string));
    
    size_t bytesCount = 0;
    uint8_t * serializedAccount = rippleAccountGetSerialization (account, &bytesCount);
    BRRippleAccount account2 = rippleAccountCreateWithSerialization(serializedAccount, bytesCount);
    char rippleAddress2[36];
    // 032BE3CEE576036F9076596FC4F8D4A2A057F8F86548C0980E021316072FEDA2D3
    // 042be3cee576036f9076596fc4f8d4a2a057f8f86548c0980e021316072feda2d35155080edffde4ce517ac8b25277494c86269bc1c7d851b276b120fe6dfd6aab
    rippleAccountGetAddressString(account2, rippleAddress2, 36);
    assert(0 == strcmp(rippleAddress2, expected_accountid_string));

    rippleAccountFree(account);
    rippleAccountFree(account2);
    free(serializedAccount);
}

static void getAccountInfo(const char* paper_key, const char* ripple_address) {
    BRRippleAccount account = createTestRippleAccount(paper_key, ripple_address);
    assert(account);
    
    // Get the 20 bytes that were created for the account
    BRRippleAddress address = rippleAccountGetAddress(account);
    for(int i = 0; i < 20; i++) {
        if (i == 0) printf("ACCOUNT ID:\n");
        printf("%02X", address.bytes[i]);
        if (i == 19) printf("\n");
    }

    BRKey publicKey = rippleAccountGetPublicKey(account);
    for(int i = 0; i < 33; i++) {
        if (i == 0) printf("Public Key:\n");
        printf("%02X", publicKey.pubKey[i]);
        if (i == 32) printf("\n");
    }
    
    rippleAccountFree(account);
}

static void
testSerializeWithSignature () {
    BRRippleTransaction transaction;

    // Here are the 20 byte addresses
    //uint8_t destBytes[] = { 0xB6, 0xF8, 0x63, 0x80, 0x8B, 0x54, 0xD6,
    //    0x43, 0xA0, 0x14, 0xCA, 0xF8, 0xB2, 0x97, 0xCF, 0xF8, 0xF2, 0xF9, 0x37, 0xE8 };
    uint8_t destBytes[] = { 0xAF, 0x65, 0x53, 0xEE, 0x2C, 0xDC, 0xA1, 0x65, 0xAB, 0x03,
        0x75, 0xA3, 0xEF, 0xB0, 0xC7, 0x65, 0x0E, 0xA5, 0x53, 0x50 };
    
    // Create an account so we can get a public key
    //const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    const char * paper_key = "F603971FCF8366465537B6AD793B37BED5FF730D3764A9DC0F7F4AD911E7372C";
    BRRippleAccount account = createTestRippleAccount(paper_key, NULL);
    // Get the 20 bytes that were created for the account
    BRRippleAddress address = rippleAccountGetAddress(account);
    char rippleAddress[36];
    rippleAccountGetAddressString(account, rippleAddress, 36);
    if (debug_log) printf("%s\n", rippleAddress);

    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    memcpy(sourceAddress.bytes, address.bytes, 20);
    memcpy(targetAddress.bytes, destBytes, 20);

    transaction = rippleTransactionCreate(sourceAddress, targetAddress, 1000000, 10);
    assert(transaction);

    uint32_t sequence_number = 25;

    // Before we sign the sequence should be 0
    uint32_t sequence = rippleTransactionGetSequence(transaction);
    // Since we don't add the sequence until later - it should be 0
    assert(0 == sequence);

    // Must set the sequence number before signing
    rippleAccountSetSequence(account, sequence_number);

    // Serialize and sign
    size_t s_size = rippleAccountSignTransaction(account, transaction, paper_key);
    size_t signed_size;
    uint8_t *signed_bytes = rippleTransactionSerialize(transaction, &signed_size);
    assert(s_size == signed_size);
    // Print out the bytes as a string
    if (debug_log) {
        for (int i = 0; i < signed_size; i++) {
            if (i == 0) printf("SIGNED BYTES\n");
            printf("%02X", signed_bytes[i]);
            if (i == (signed_size -1)) printf("\n");
        }
    }

    // Now that it is serialized/signed - check out the fee
    BRRippleUnitDrops fee =  rippleTransactionGetFee(transaction);
    assert(10 == fee);

    // After calling the sign function the sequence number should match what we passed in
    sequence = rippleTransactionGetSequence(transaction);
    // Since we don't add the sequence until later - it should be 0
    assert(sequence_number == sequence);

    // Compare the output with what we are expecting - ignore the signature
    //assert(0 == memcmp(signed_bytes, expected_output, 50));
    // Now compare the last 2 fields (source and destination)
    //assert(0 == memcmp(&signed_bytes[signed_size-44], &expected_output[sizeof(expected_output)-44], 44));
    
    rippleTransactionFree(transaction);
    rippleAccountFree(account);
    free(signed_bytes);
}

extern BRRippleSignatureRecord rippleTransactionGetSignature(BRRippleTransaction transaction);

static BRRippleTransaction transactionDeserialize(const char * trans_bytes, const char * extra_fields)
{
    // Combine the transaction string (hex string of serialized transaction) along with
    // any extra bytes.  The extra bytes would be fields that we currently don't know how
    // to parse.
    size_t tx_length = strlen(trans_bytes) + 1;
    if (extra_fields) {
        tx_length += strlen(extra_fields);
    }
    char tx_blob[tx_length];
    memset(tx_blob, 0x00, sizeof(tx_blob));
    strcpy(tx_blob, trans_bytes);
    if (extra_fields) {
        strcat(tx_blob, extra_fields);
    }

    // Convert the trans to binary
    uint8_t bytes[strlen(tx_blob)/2];
    memset(bytes,0x00, sizeof(bytes));
    hex2bin(tx_blob, bytes);

    BRRippleTransaction transaction = rippleTransactionCreateFromBytes(bytes, (int)sizeof(bytes));
    return transaction;
}

const char * serialized_transaction = "120000"
    "2280000000"
    "2400000019"
    "6140000000000F4240"
    "68400000000000000A"
    "7321035590938D3FDA530A36DBA666C463530D830387ED68F7F6C40B38EC922C0A0885"
    "74463044022005CF72B172AAA5AA326AFA7FC90D2B3DBD0EDD9E778DA44ACC380BEAAC"
    "8BF46F02207C19625D87CCCDD29688780F95CD895913BED9F0415B92F17A1799A2CE19F258"
    "81140A000DC76DC6E5843BDBE06A274E90C8A6B4AC2C"
    "8314AF6553EE2CDCA165AB0375A3EFB0C7650EA55350";

static void validateDeserializedTransaction(BRRippleTransaction transaction)
{
    assert(transaction);
    uint16_t transactionType = rippleTransactionGetType(transaction);
    assert(RIPPLE_TX_TYPE_PAYMENT == transactionType);
    
    uint32_t sequence = rippleTransactionGetSequence(transaction);
    assert(25 == sequence);
    
    // Validate the source address
    uint8_t expectedSource[20];
    hex2bin("0A000DC76DC6E5843BDBE06A274E90C8A6B4AC2C", expectedSource);
    BRRippleAddress source = rippleTransactionGetSource(transaction);
    assert(0 == memcmp(source.bytes, expectedSource, 20));
    
    // Validate the destination address
    uint8_t expectedTarget[20];
    hex2bin("AF6553EE2CDCA165AB0375A3EFB0C7650EA55350", expectedTarget);
    BRRippleAddress destination = rippleTransactionGetTarget(transaction);
    assert(0 == memcmp(destination.bytes, expectedTarget, 20));
    
    // Public key from the tx_blob
    uint8_t expectedPubKey[33];
    hex2bin("035590938D3FDA530A36DBA666C463530D830387ED68F7F6C40B38EC922C0A0885", expectedPubKey);
    BRKey pubKey = rippleTransactionGetPublicKey(transaction);
    assert(0 == (memcmp(expectedPubKey, pubKey.pubKey, 33)));
    
    uint64_t fee = rippleTransactionGetFee(transaction);
    assert(10 == fee); // The fee is valid for deserialized transactions
    
    uint32_t flags = rippleTransactionGetFlags(transaction);
    assert(0x80000000 == flags);
}

static void validateOptionalFields(BRRippleTransaction transaction,
                                   uint32_t expectedSourceTag,
                                   uint32_t expectedDestinationTag,
                                   UInt256 *expectedInvoiceId,
                                   BRRippleTransactionHash *expectedAccountTxnId)
{
    assert(transaction);

    // source tag
    uint32_t sourceTag = rippleTransactionGetSourceTag(transaction);
    assert(expectedSourceTag == sourceTag);

    // destination tag
    uint32_t destinationTag = rippleTransactionGetDestinationTag(transaction);
    assert(expectedDestinationTag == destinationTag);
    
    UInt256 invoiceId = rippleTransactionGetInvoiceID(transaction);
    assert(0 == memcmp(expectedInvoiceId->u8, invoiceId.u8, 32));

    BRRippleTransactionHash accountTxnId = rippleTransactionGetAccountTxnId(transaction);
    assert(0 == memcmp(expectedAccountTxnId->bytes, accountTxnId.bytes, 32));
}

static void testTransactionDeserialize()
{
    // This is a real tx blob coming back from ripple test server
    BRRippleTransaction transaction = transactionDeserialize(serialized_transaction, NULL);
    validateDeserializedTransaction(transaction);
    UInt256 expectedInvoiceId;
    memset(expectedInvoiceId.u8, 0x00, sizeof(expectedInvoiceId.u8));
    BRRippleTransactionHash expectedAccountTxnId;
    memset(expectedAccountTxnId.bytes, 0x00, sizeof(expectedAccountTxnId.bytes));
    validateOptionalFields(transaction, 0, 0,
                           &expectedInvoiceId, &expectedAccountTxnId);
    
    rippleTransactionFree(transaction);
}

static void testTransactionDeserializeUnknownFields()
{
    // Test with the STObject appended
    BRRippleTransaction transaction = transactionDeserialize(serialized_transaction, "E000");
    validateDeserializedTransaction(transaction);
    rippleTransactionFree(transaction);

    // Test with the STArray
    transaction = transactionDeserialize(serialized_transaction, "F000");
    validateDeserializedTransaction(transaction);
    rippleTransactionFree(transaction);

    // Test with the PathSet field appended
    transaction = transactionDeserialize(serialized_transaction, "0112");
    validateDeserializedTransaction(transaction);
    rippleTransactionFree(transaction);

}

static void testTransactionDeserializeOptionalFields()
{
    const char * test_input = "1200002280000000240000000261D4838D7EA4C6800000000000000000000000000055534400000000004B4E9C06F24296074F7BC48F92A97916C6DC5EA968400000000000000C69D4838D7EA4C6800000000000000000000000000055534400000000004B4E9C06F24296074F7BC48F92A97916C6DC5EA981144B4E9C06F24296074F7BC48F92A97916C6DC5EA983143E9D4A2B8AA0780F682D136F7A56D6724EF53754";

    BRRippleTransaction transaction = transactionDeserialize(test_input, NULL);
    assert(transaction);

    BRRippleAmount sendMax = rippleTransactionGetAmountRaw(transaction, RIPPLE_AMOUNT_TYPE_SENDMAX);
    assert(1 == sendMax.currencyType);

    BRRippleAmount amountRaw = rippleTransactionGetAmountRaw(transaction, RIPPLE_AMOUNT_TYPE_AMOUNT);
    assert(1 == amountRaw.currencyType);
    // Not sure how to test doubles
    //assert((double)1 == amountRaw.amount.dAmount);
}

static void testTransactionDeserializeLastLedgerSequence()
{
    // 201B0121EAC0
    const char * test_input = "1200002400000001201B0121EAC06140000000000F424068400000000000000C81142D66C01F69269EE58D62174B821295CCF763DF3C8314572A19A7C7DD6F72C7D24DE3F10FDFEFE8636A36";

    BRRippleTransaction transaction = transactionDeserialize(test_input, NULL);
    assert(transaction);

    uint32_t lastLedgerSequence = rippleTransactionGetLastLedgerSequence(transaction);
    assert(19000000 == lastLedgerSequence);
}

static void testTransactionDeserializeWithMemos()
{
    const char * test_input = "1200002400000001201B0121EAC06140000000000F424068400000000000000C"
    "81142D66C01F69269EE58D62174B821295CCF763DF3C"
    "8314572A19A7C7DD6F72C7D24DE3F10FDFEFE8636A36"
    "F9EA7C1F687474703A2F2F6578616D706C652E636F6D2F6"
    "D656D6F2F67656E657269637D04AAAAAAAAE1"
    "EA7C1F687474703A2F2F6578616D706C652E636F6D2F6D656D6F2F67656E657269637D04BBBBBBBBE1F1";

    BRRippleTransaction transaction = transactionDeserialize(test_input, NULL);
    assert(transaction);
    
    uint32_t lastLedgerSequence = rippleTransactionGetLastLedgerSequence(transaction);
    assert(19000000 == lastLedgerSequence);

    rippleTransactionFree(transaction);
}

static void testTransactionDeserialize1(const char* test_input)
{
    BRRippleTransaction transaction = transactionDeserialize(test_input, NULL);
    assert(transaction);

    // Check if this is a transaction
    BRRippleTransactionType txType = rippleTransactionGetType(transaction);
    assert(txType == RIPPLE_TX_TYPE_PAYMENT);

    // Get the sequence number
    BRRippleSequence sequence = rippleTransactionGetSequence(transaction);

    // Record the currency type
    BRRippleAmount amount = rippleTransactionGetAmountRaw(transaction, RIPPLE_AMOUNT_TYPE_AMOUNT);
    printf("sequence: %u, type: %s\n", sequence, (amount.currencyType == 0 ? "XRP" : "OTHER"));

    rippleTransactionFree(transaction);
}

// Get test data from here:
// https://data.ripple.com/v2/transactions/?descending=false&limit=1&type=Payment&binary=true
// set desending to true to get the lateset

const char * tx_one = "120000228007000024002BD71F201B02CF99B861D5838D7EA4C6800000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC35068400000000000000B6940000002540BE4007321030AC4F2BA6E1FF86BEB234B639918DAFDF0675032AE264D2B39641503822373FE74473045022100DA106899FF0529AD993600640AE1A38F491144FAE4F8731B5C35DB41E16013FB02204357DCFB8F2030F09937E58A16E2D96A52AFF47BF5E714E2B24E8968555BFE0F8114C90B9B6694BFD005C17B7217A1F38676BDD30F8D8314C90B9B6694BFD005C17B7217A1F38676BDD30F8D011230000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A30000000000000000000000000554C540000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A1000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC350FF30000000000000000000000000554C540000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A30000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A1000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC350FF30000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A30000000000000000000000000584C4D0000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A1000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC350FF30000000000000000000000000584C4D0000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A30000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A1000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC350FF30000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A3000000000000000000000000055534400000000000A20B3C85F482532A9578DBB3950B85CA06594D11000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC350FF3000000000000000000000000055534400000000000A20B3C85F482532A9578DBB3950B85CA06594D130000000000000000000000000434E590000000000CED6E99370D5C00EF4EBF72567DA99F5661BFB3A1000000000000000000000000000000000000000003000000000000000000000000058434E0000000000FA46CF7AFBD47F59DE3B6CD5379A7BAFB88AC35000";

const char * tx_two = "1200002200000000240000003E6140000002540BE40068400000000000000A7321034AADB09CFF4A4804073701EC53C3510CDC95917C2BB0150FB742D0C66E6CEE9E74473045022022EB32AECEF7C644C891C19F87966DF9C62B1F34BABA6BE774325E4BB8E2DD62022100A51437898C28C2B297112DF8131F2BB39EA5FE613487DDD611525F17962646398114550FC62003E785DC231A1058A05E56E3F09CF4E68314D4CC8AB5B21D86A82C3E9E8D0ECF2404B77FECBA";

static void
testTransactionHash (void /* ... */) {
    BRRippleTransaction transaction;

    // Here are the 20 byte addresses
    uint8_t destBytes[] = { 0xAF, 0x65, 0x53, 0xEE, 0x2C, 0xDC, 0xA1, 0x65, 0xAB, 0x03,
        0x75, 0xA3, 0xEF, 0xB0, 0xC7, 0x65, 0x0E, 0xA5, 0x53, 0x50 };
    
    // Create an account so we can get a public key
    const char * paper_key = "F603971FCF8366465537B6AD793B37BED5FF730D3764A9DC0F7F4AD911E7372C";
    BRRippleAccount account = createTestRippleAccount(paper_key, NULL);
    // Get the 20 bytes that were created for the account
    BRRippleAddress address = rippleAccountGetAddress(account);
    
    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    memcpy(sourceAddress.bytes, address.bytes, 20);
    memcpy(targetAddress.bytes, destBytes, 20);
    
    transaction = rippleTransactionCreate(sourceAddress, targetAddress, 1000000, 12);
    assert(transaction);

    // Serialize and sign
    rippleAccountSetSequence(account, 25);
    rippleAccountSignTransaction(account, transaction, paper_key);
    
    // Compare the transaction hash
    uint8_t expected_hash[] = {
        0xC0, 0x33, 0x01, 0x0E, 0x2A, 0x32, 0x69, 0xBF,
        0x21, 0x2A, 0xDB, 0x62, 0x81, 0x58, 0xD8, 0xB6,
        0x89, 0x7D, 0x15, 0x53, 0x54, 0x8F, 0xCB, 0xBD,
        0x9A, 0x2A, 0xC7, 0x43, 0xED, 0xB3, 0x09, 0x0F
    };
    BRRippleTransactionHash hash = rippleTransactionGetHash(transaction);
    assert(0 == memcmp(hash.bytes, expected_hash, 32));

    rippleTransactionFree(transaction);
    rippleAccountFree(account);
}


static void createAndDeleteWallet()
{
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRRippleAccount account = rippleAccountCreate(paper_key);
    BRRippleWallet wallet = rippleWalletCreate(account);
    assert(wallet);
    rippleWalletFree(wallet);
}

static void testWalletValues()
{
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRRippleAccount account = rippleAccountCreate(paper_key);
    BRRippleWallet wallet = rippleWalletCreate(account);
    assert(wallet);

    uint64_t balance = rippleWalletGetBalance(wallet);
    assert(balance == 0);

    uint64_t expected_balance = 25000000;
    rippleWalletSetBalance(wallet, expected_balance);
    balance = rippleWalletGetBalance(wallet);
    assert(balance == expected_balance);

    BRRippleFeeBasis feeBasis = rippleWalletGetDefaultFeeBasis(wallet);
    assert(0 == feeBasis);
    rippleWalletSetDefaultFeeBasis(wallet, 10);
    feeBasis = rippleWalletGetDefaultFeeBasis(wallet);
    assert(10 == feeBasis);

    rippleWalletFree(wallet);
}

static void testWalletAddress()
{
    const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRRippleAccount account = rippleAccountCreate(paper_key);
    BRRippleWallet wallet = rippleWalletCreate(account);
    assert(wallet);

    uint8_t accountBytes[] = { 0xEF, 0xFC, 0x27, 0x52, 0xB5, 0xC9, 0xDA, 0x22, 0x88, 0xC5,
        0xD0, 0x1F, 0x30, 0x4E, 0xC8, 0x29, 0x51, 0xE3, 0x7C, 0xA2 };

    BRRippleAddress sourceAddress = rippleWalletGetSourceAddress(wallet);
    assert(0 == memcmp(accountBytes, sourceAddress.bytes, 20));

    BRRippleAddress targetAddress = rippleWalletGetTargetAddress(wallet);
    assert(0 == memcmp(accountBytes, targetAddress.bytes, 20));

    rippleWalletFree(wallet);
}

static void testRippleAddressCreate()
{
    uint8_t expected_bytes[] = { 0xEF, 0xFC, 0x27, 0x52, 0xB5, 0xC9, 0xDA, 0x22, 0x88, 0xC5,
        0xD0, 0x1F, 0x30, 0x4E, 0xC8, 0x29, 0x51, 0xE3, 0x7C, 0xA2 };
    const char* ripple_address = "r41vZ8exoVyUfVzs56yeN8xB5gDhSkho9a";
    
    BRRippleAddress address = rippleAddressCreate(ripple_address);
    assert(0 == memcmp(expected_bytes, address.bytes, 20));
}

static void testRippleAddressEqual()
{
    uint8_t address_input[] = { 0xEF, 0xFC, 0x27, 0x52, 0xB5, 0xC9, 0xDA, 0x22, 0x88, 0xC5,
        0xD0, 0x1F, 0x30, 0x4E, 0xC8, 0x29, 0x51, 0xE3, 0x7C, 0xA2 };

    BRRippleAddress a1;
    BRRippleAddress a2;
    memcpy(a1.bytes, address_input, 20);
    a2 = a1;
    int result = rippleAddressEqual(a1, a2);
    assert(1 == result);

    // Modify a2
    a2.bytes[0] = 0xFF;
    result = rippleAddressEqual(a1, a2);
    assert(0 == result);
}

void rippleAccountTests()
{
    testCreateRippleAccountWithPaperKey();
    testCreateRippleAccountWithSeed();
    testCreateRippleAccountWithKey();
    testCreateRippleAccountWithSerializedBytes();
    testRippleAddressCreate();
    testRippleAddressEqual();
}

void rippleTransactionTests()
{
    testRippleTransaction();
    testRippleTransactionGetters();
    testSerializeWithSignature();
    testTransactionHash();
    testTransactionDeserialize();
    testTransactionDeserializeUnknownFields();
    testTransactionDeserializeOptionalFields();
    testTransactionDeserializeLastLedgerSequence();
    testTransactionDeserializeWithMemos();
    testTransactionDeserialize1(tx_one);
    testTransactionDeserialize1(tx_two);
}

static void runWalletTests()
{
    createAndDeleteWallet();
    testWalletValues();
    testWalletAddress();
}

static void comparebuffers(const char *input, uint8_t * output, size_t outputSize)
{
    size_t input_size = strlen(input) / 2;
    assert(input_size == outputSize);
    uint8_t buffer[input_size];
    hex2bin(input, buffer);
    for(int i = 0; i < outputSize; i++) {
        assert(buffer[i] == output[i]);
    }
}
static void runDeserializeTests(const char* tx_list_name, const char* tx_list[], int num_elements)
{
    int payments = 0;
    int other_type = 0;
    int xrp_currency = 0;
    int other_currency = 0;
    for(int i = 0; i <= num_elements - 2; i += 2) {
        size_t input_size = strlen(tx_list[i+1]) / 2;
        size_t output_size;
        BRRippleTransaction transaction = transactionDeserialize(tx_list[i+1], NULL);
        uint8_t * signed_bytes = rippleTransactionSerialize(transaction, &output_size);
        assert(input_size == output_size);
        comparebuffers(tx_list[i+1], signed_bytes, output_size);
        free(signed_bytes);

        assert(transaction);
        
        // Check if this is a transaction
        BRRippleTransactionType txType = rippleTransactionGetType(transaction);
        if (txType == RIPPLE_TX_TYPE_PAYMENT) {
            // Get the sequence number
            BRRippleSequence sequence = rippleTransactionGetSequence(transaction);
            assert(0 != sequence);
            // Record the currency type
            BRRippleAmount amount = rippleTransactionGetAmountRaw(transaction, RIPPLE_AMOUNT_TYPE_AMOUNT);
            if (amount.currencyType == 0) {
                xrp_currency++;
            } else {
                other_currency++;
            }
            payments++;
        } else {
            other_type++;
        }
        
        rippleTransactionFree(transaction);
    }
    printf("list_name: %s, tx count: %d, payments: %d, other: %d, XRP: %d, other currency: %d\n",
           tx_list_name, num_elements/2, payments, other_type, xrp_currency, other_currency);
}

extern void
runRippleTest (void /* ... */) {

    // Read data from external file and deserialize
    runDeserializeTests("200 new transaction", test_tx_list, (int)(sizeof(test_tx_list)/sizeof(char*)));
    runDeserializeTests("200 old transactions", test_tx_list2, (int)(sizeof(test_tx_list2)/sizeof(char*)));

    // Account tests
    rippleAccountTests();

    // Transaction tests
    rippleTransactionTests();

    // Wallet tests
    runWalletTests();

    // getAccountInfo is just a utility to print out info if needed
    const char* paper_key = "996F8505F48FCB74D291F7B459A5EA20FEA97ACEC803D7459B3742AF3DCF4B9D";
    const char* ripple_address = "rGzQdxGFHLtiP16x8k458Am5sy8mBcnhpD";
    getAccountInfo(paper_key, ripple_address);
}
