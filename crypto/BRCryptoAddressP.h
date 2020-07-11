//
//  BRCryptoAddressP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoAddressP_h
#define BRCryptoAddressP_h

#include "BRCryptoBaseP.h"
#include "BRCryptoAddress.h"

#include "bcash/BRBCashAddr.h"
#include "support/BRAddress.h"
#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BRCryptoAddressRecord {
    BRCryptoBlockChainType type;
    union {
        /// A BTC or BCH address
        struct {
            // `true` if BTC; `false` if `BCH`
            BRCryptoBoolean isBitcoinAddr;

            /// The 'bitcoin/' address.  For BTC, addr.s is the string; for BCH, addr.s is
            /// encoded in a 'BCH' specific way.
            BRAddress addr;
        } btc;

        /// A ETH address
        BREthereumAddress eth;

        /// A GEN address
        BRGenericAddress gen;
    } u;
    BRCryptoRef ref;
};

 private_extern BRCryptoAddress
 cryptoAddressCreateAsBTC (BRAddress btc,
                           BRCryptoBoolean isBTC);  // TRUE if BTC; FALSE if BCH

 private_extern BRCryptoAddress
 cryptoAddressCreateAsETH (BREthereumAddress eth);

 private_extern BRCryptoAddress
 cryptoAddressCreateAsGEN (OwnershipGiven BRGenericAddress gen);

 private_extern BRCryptoBlockChainType
 cryptoAddressGetType (BRCryptoAddress address);

 private_extern BRAddress
 cryptoAddressAsBTC (BRCryptoAddress address,
                     BRCryptoBoolean *isBitcoinAddr);

 private_extern BREthereumAddress
 cryptoAddressAsETH (BRCryptoAddress address);

 private_extern BRGenericAddress
 cryptoAddressAsGEN (BRCryptoAddress address);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAddressP_h */
