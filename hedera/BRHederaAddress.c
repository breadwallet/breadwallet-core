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
    address->account = 0; // Account 0.0.0 will be the __fee__ address
    return address;
}

extern char * hederaAddressAsString (BRHederaAddress address)
{
    assert(address);
    char * string = NULL;

    // Check for our special case __fee__ address
    // See the note above with respect to the feeAddressBytes
    if (address->account == 0) {
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

BRHederaAddress hederaAddressStringToAddress(const char* input)
{
    // Hedera address are shard.realm.account
    BRHederaAddress address = (BRHederaAddress) calloc(1, sizeof(struct BRHederaAddressRecord));
    sscanf(input, "%lld.%lld.%lld",
           &address->shard, &address->realm, &address->account);
    return address;
}

extern BRHederaAddress
hederaAddressCreateFromString(const char * hederaAddressString)
{
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
    if (address->account == 0) {
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

