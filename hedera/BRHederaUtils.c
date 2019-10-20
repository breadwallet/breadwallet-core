/**
*/

#include "BRHederaUtils.h"
#include <stdio.h>
#include <assert.h>
#include <memory.h>

int hederaAddressGetString(BRHederaAddress address, char * addressString, int length)
{
    // The account is made up of 3 int64_t values. The string
    // representation is shard.realm.account so the max length
    // would be 20 digits per number, plus 2 periods plus a terminating byte
    // To make is simple just have a 128 byte buffer
    assert(length > 63);
    assert(addressString);
    return sprintf(addressString, "%lld.%lld.%lld", address.shard, address.realm, address.account);
}

BRHederaTransactionId hederaParseTransactionId (const char * txId)
{
    assert(txId);
    BRHederaTransactionId id;
    memset(&id, 0x00, sizeof(BRHederaTransactionId));
    // 0.0.14623-1568420904-460838529
    sscanf(txId, "%lld.%lld.%lld-%lld-%d",
           &id.address.shard, &id.address.realm, &id.address.account,
           &id.timeStamp.seconds, &id.timeStamp.nano);
    return id;
}
