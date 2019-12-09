//
//  BRHederaAddress.c
//  Core
//
//  Created by Carl Cherry on Oct. 21, 2019.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include "BRHederaAddress.h"
#include "support/BRCrypto.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <stdbool.h>

struct BRHederaAddressRecord {
    int64_t shard;
    int64_t realm;
    int64_t account;
};

extern void hederaAddressFree (BRHederaAddress address)
{
    if (address) free(address);
}

BRHederaAddress hederaAddressCreateFeeAddress()
{
    BRHederaAddress address = calloc(1, sizeof(struct BRHederaAddressRecord));
    address->shard  = -1;
    address->realm  = -1;
    address->account = -1;
    return address;
}

extern char * hederaAddressAsString (BRHederaAddress address)
{
    assert(address);
    char * string = NULL;

    // Check for our special case __fee__ address
    // See the note above with respect to the feeAddressBytes
    if (address->account == -1) {
        string = calloc(1, 8);
        strcpy(string, "__fee__");
    } else {
        // Hedera addresses are shown as a.b.c
        char buffer[1024];
        memset(buffer, 0x00, sizeof(buffer));
        size_t stringSize = sprintf(buffer, "%lld.%lld.%lld", address->shard, address->realm, address->account);
        assert(stringSize > 0);
        string = calloc(1, stringSize + 1);
        strcpy(string, buffer);
    }
    return string;
}

static bool validInt64Value(int64_t value, const char* input) {
    // If the value is equal to the max then perhaps the input
    // was a number larger than the max
    if (value == INT64_MAX) {
        // Check to see if the string input is too large
        if (strcmp("9223372036854775807", input) != 0) {
            return false;
        }
    }
    return true;
}

static bool hederaStringIsValid (const char * input)
{
    bool valid = false;
    char * signChar = strstr(input, "-");
    if (signChar) {
        // No negative account numbers allowed.
        return false;
    }
    char * testInput = strdup(input);
    char * shard = strtok(testInput, ".");
    if (shard && strlen(shard) <= 19) {
        char * realm = strtok(NULL, ".");
        if (realm && strlen(realm) <= 19) {
            char * account = strtok(NULL, ".");
            if (account && strlen(account) <= 19) {
                // Now we have all 3 values
                // Check if any of them
                int64_t num1 = 0, num2 = 0, num3 = 0;
                sscanf(input, "%lld.%lld.%lld", &num1, &num2, &num3);
                if (validInt64Value(num1, shard)
                    && validInt64Value(num2, realm)
                    && validInt64Value(num3, account)) {
                    valid = true;
                }
            }
        }
    }

    free (testInput);
    return valid;
}

BRHederaAddress hederaAddressStringToAddress(const char* input)
{
    if (!hederaStringIsValid(input)) {
        return NULL;
    }

    // Hedera address are shard.realm.account
    BRHederaAddress address = (BRHederaAddress) calloc(1, sizeof(struct BRHederaAddressRecord));
    sscanf(input, "%lld.%lld.%lld",
           &address->shard, &address->realm, &address->account);
    return address;
}

extern BRHederaAddress
hederaAddressCreateFromString(const char * hederaAddressString)
{
    assert(hederaAddressString);

    if (!hederaAddressString) {
        return NULL;
    }

    // 1 special case so far - the __fee__ address.
    // See the note in BRHederaAcount.h with respect to the feeAddressBytes
    if (strcmp(hederaAddressString, "__fee__") == 0) {
        return hederaAddressCreateFeeAddress ();
    } else {
        // Work backwards from this hedera address (string) to what is
        // known as the acount ID (20 bytes)
        return hederaAddressStringToAddress (hederaAddressString);
    }
}

extern int // 1 if equal
hederaAddressEqual (BRHederaAddress a1, BRHederaAddress a2) {
    return (a1->shard == a2->shard &&
            a1->realm == a2->realm &&
            a1->account == a2->account);
}

extern int
hederaAddressIsFeeAddress (BRHederaAddress address)
{
    assert(address);
    if (address->shard == -1 && address->realm == -1 && address->account == -1) {
        return 1;
    } else {
        return 0;
    }
}

extern BRHederaAddress hederaAddressClone (BRHederaAddress address)
{
    if (address) {
        BRHederaAddress clone = calloc(1, sizeof(struct BRHederaAddressRecord));
        *clone = *address;
        return clone;
    }
    return NULL;
}

int64_t hederaAddressGetShard (BRHederaAddress address)
{
    assert (address);
    return address->shard;
}

int64_t hederaAddressGetRealm (BRHederaAddress address)
{
    assert (address);
    return address->realm;
}

int64_t hederaAddressGetAccount (BRHederaAddress address)
{
    assert (address);
    return address->account;
}

inline static void Int64SetBE(void *b8, int64_t u)
{
    *(union _u64 { uint8_t u8[64/8]; } *)b8 =
        (union _u64) { (u >> 56) & 0xff, (u >> 48) & 0xff, (u >> 40) & 0xff, (u >> 32) & 0xff,
                       (u >> 24) & 0xff, (u >> 16) & 0xff, (u >> 8) & 0xff, u & 0xff };
}

inline static int64_t Int64GetBE(const void *b8)
{
    return (((int64_t)((const uint8_t *)b8)[0] << 56) | ((int64_t)((const uint8_t *)b8)[1] << 48) |
            ((int64_t)((const uint8_t *)b8)[2] << 40) | ((int64_t)((const uint8_t *)b8)[3] << 32) |
            ((int64_t)((const uint8_t *)b8)[4] << 24) | ((int64_t)((const uint8_t *)b8)[5] << 16) |
            ((int64_t)((const uint8_t *)b8)[6] << 8)  | ((int64_t)((const uint8_t *)b8)[7]));
}

extern uint8_t * hederaAddressSerialize (BRHederaAddress address, size_t * sizeOfBytes)
{
    assert(address);
    assert(sizeOfBytes);
    *sizeOfBytes = sizeof(int64_t) * 3;
    uint8_t * bytes = calloc(1, *sizeOfBytes);

    // For simplicity we can
    Int64SetBE(bytes, address->shard);
    Int64SetBE(bytes + sizeof(int64_t), address->realm);
    Int64SetBE(bytes + 2 * sizeof(int64_t), address->account);
    return bytes;
}

extern BRHederaAddress hederaAddressCreateFromBytes (uint8_t * bytes, size_t sizeOfBytes)
{
    assert(bytes);
    assert(sizeOfBytes == sizeof(int64_t) * 3);
    BRHederaAddress address = (BRHederaAddress) calloc(1, sizeof(struct BRHederaAddressRecord));
    address->shard = Int64GetBE(bytes);
    address->realm = Int64GetBE(bytes + sizeof(int64_t));
    address->account = Int64GetBE(bytes + 2 * sizeof(int64_t));
    return address;
}

