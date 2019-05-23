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
        *(target++) = (char2int(src[0]) << 4) | (char2int(src[1]) & 0xff);
        src += 2;
    }
}

static void runAccountTests()
{
const char * paper_key = "patient doctor olympic frog force glimpse endless antenna online dragon bargain someone";
    BRStellarAccount account = stellarAccountCreate(paper_key);
    
    const char* public_key_string = "5562f344b6471448b7b6ebeb5bae9c1cecc930ef28868be2bb78bb742831e710";
    uint8_t expected_public_key[32];
    hex2bin(public_key_string, expected_public_key);
    BRKey key = stellarAccountGetPublicKey(account);
    assert(0 == memcmp(key.pubKey, expected_public_key, sizeof(expected_public_key)));

    BRStellarAddress address = stellarAccountGetAddress(account);
    const char* expected_address = "GBKWF42EWZDRISFXW3V6WW5OTQOOZSJQ54UINC7CXN4LW5BIGHTRB3BB";
    printf("stellar address: %s\n", address.bytes);
    assert(0 == memcmp(address.bytes, expected_address, strlen(expected_address)));

    stellarAccountFree(account);
}


void decodeXRDPayment() {
    const char * xdr = "AAAAACQP/rfPQXGBsLCTIDX4vAhrBNFsGLHbjGKfEQXiaHrRAAAAyAAHHCYAAAABAAAAAAAAAAEAAAAUQnV5IHlvdXJzZWxmIGEgYmVlciEAAAACAAAAAAAAAAEAAAAAVWLzRLZHFEi3tuvrW66cHOzJMO8ohoviu3i7dCgx5xAAAAAAAAAAAAZCLEAAAAAAAAAABQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAC2ZlZC5uZXR3b3JrAAAAAAAAAAAAAAAAAeJoetEAAABA6kZvVPyCaTTzDTPQ7noWJtJ+byyADdVLQp8IQhpA45qsduJ1kyxtVbxkvDSBMG6LEkqhEu3ScxXM39WKWRKbBQ==";

    size_t decodeSize = 0;
    unsigned char* decoded = b64_decode_ex(xdr, strlen(xdr), &decodeSize);
    for(int i = 0; i < decodeSize; i++) {
        if (i % 10 == 0) printf("\n");
        printf("%02X ", decoded[i]);
    }
    printf("\n");
}

extern void
runStellarTest (void /* ... */) {
    decodeXRDPayment();
    runAccountTests();
}
