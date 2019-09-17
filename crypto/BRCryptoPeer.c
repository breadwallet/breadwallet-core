//
//  BRCryptoPeer.h
//  BRCore
//
//  Created by Ed Gamble on 10/17/2019.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoPeer.h"
#include <arpa/inet.h>

/** Forward Declarations */
static void
cryptoPeerRelease (BRCryptoPeer peer);

struct BRCryptoPeerRecord {
    BRCryptoNetwork network;
    char *address;
    uint16_t port;
    char *publicKey;

    struct in_addr inetAddr;

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
    struct in_addr addr;
    if (-1 == ascii2addr (AF_INET, address, &addr)) return NULL;

    BRCryptoPeer peer = calloc (1, sizeof (struct BRCryptoPeerRecord));

    peer->network   = cryptoNetworkTake (network);
    peer->address   = strdup (address);
    peer->port      = port;
    peer->publicKey = (NULL == publicKey ? NULL : strdup(publicKey));
    peer->inetAddr  = addr;
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

    free (peer);
}

private_extern struct in_addr
cryptoPeerGetInetAddr (BRCryptoPeer peer) {
    return peer->inetAddr;
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
