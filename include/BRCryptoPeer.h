//
//  BRCryptoPeer.h
//  BRCore
//
//  Created by Ed Gamble on 10/17/2019.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoPeer_h
#define BRCryptoPeer_h

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoPeerRecord *BRCryptoPeer;

    private_extern BRCryptoPeer
    cryptoPeerCreate (BRCryptoNetwork network,
                      const char *address,
                      uint16_t port,
                      const char *publicKey);

    private_extern BRCryptoPeer
    cryptoPeerCreateFromSerialization (BRCryptoNetwork network,
                                       uint8_t *bytes, size_t bytesCount);

    extern BRCryptoData16
    cryptoPeerGetAddrAsInt (BRCryptoPeer peer);

    extern BRCryptoNetwork
    cryptoPeerGetNetwork (BRCryptoPeer peer);

    extern const char *
    cryptoPeerGetAddress (BRCryptoPeer peer);

    extern uint16_t
    cryptoPeerGetPort (BRCryptoPeer peer);

    extern const char *
    cryptoPeerGetPublicKey (BRCryptoPeer peer);

    extern uint8_t *
    cryptoPeerSerialize (BRCryptoPeer peer, size_t *bytesCount);

    extern BRCryptoBoolean
    cryptoPeerIsIdentical (BRCryptoPeer p1, BRCryptoPeer p2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoPeer, cryptoPeer);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoPeer_h */
