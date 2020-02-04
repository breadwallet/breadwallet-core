//
//  BRCryptoPeer.h
//  BRCore
//
//  Created by Ed Gamble on 10/17/2019.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <arpa/inet.h>
#include "support/BRArray.h"
#include "support/BRInt.h"
#include "BRCryptoPeer.h"

struct BRCryptoPeerRecord {
    BRCryptoNetwork network;
    char *address;
    uint16_t port;
    char *publicKey;

    // Address parsed as AF_INET or AF_INET6
    UInt128 addressAsInt;

    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoPeer, cryptoPeer);

private_extern BRCryptoPeer
cryptoPeerCreate (BRCryptoNetwork network,
                  const char *address,
                  uint16_t port,
                  const char *publicKey) {
    // Require `address`
    if (NULL == address || 0 == strlen (address)) return NULL;

    // Require `address` to parse as a INET address
    UInt128 addressAsInt = UINT128_ZERO;

    struct in_addr addr;
    struct in6_addr addr6;

    if (1 == inet_pton (AF_INET6, address, &addr6)) {
        addressAsInt = *((UInt128*) &addr6.s6_addr);
    }
    else if (1 == inet_pton (AF_INET, address, &addr)) {
        addressAsInt.u16[5] = 0xffff;
        addressAsInt.u32[3] = addr.s_addr;
    }
    else return NULL;

    BRCryptoPeer peer = calloc (1, sizeof (struct BRCryptoPeerRecord));

    peer->network   = cryptoNetworkTake (network);
    peer->address   = strdup (address);
    peer->port      = port;
    peer->publicKey = (NULL == publicKey ? NULL : strdup(publicKey));
    peer->addressAsInt = addressAsInt;
    peer->ref       = CRYPTO_REF_ASSIGN (cryptoPeerRelease);

    return peer;
}

private_extern BRCryptoPeer
cryptoPeerCreateFromSerialization (BRCryptoNetwork network,
                                   uint8_t *bytes, size_t bytesCount) {
    return NULL;
}

static void
cryptoPeerRelease (BRCryptoPeer peer) {
    cryptoNetworkGive (peer->network);
    free (peer->address);
    if (NULL != peer->publicKey) free (peer->publicKey);

    memset (peer, 0, sizeof(*peer));
    free (peer);
}

extern BRCryptoData16
cryptoPeerGetAddrAsInt (BRCryptoPeer peer) {
    BRCryptoData16 addrAsInt;
    memcpy (addrAsInt.data, peer->addressAsInt.u8, sizeof (addrAsInt.data));
    return addrAsInt;
}

extern BRCryptoNetwork
cryptoPeerGetNetwork (BRCryptoPeer peer) {
    return cryptoNetworkTake (peer->network);
}

extern const char *
cryptoPeerGetAddress (BRCryptoPeer peer) {
    return peer->address;
}

extern uint16_t
cryptoPeerGetPort (BRCryptoPeer peer) {
    return peer->port;
}

extern const char *
cryptoPeerGetPublicKey (BRCryptoPeer peer) {
    return peer->publicKey;
}

extern uint8_t *
cryptoPeerSerialize (BRCryptoPeer peer, size_t *bytesCount) {
    assert (NULL != bytesCount);
    *bytesCount = 0;
    return NULL;
}

extern BRCryptoBoolean
cryptoPeerIsIdentical (BRCryptoPeer p1, BRCryptoPeer p2) {
    return AS_CRYPTO_BOOLEAN (p1 == p2);
}
