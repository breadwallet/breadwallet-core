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

#include "BRCryptoBase.h"
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
        struct {
            BRCryptoBoolean isBitcoinAddr; // TRUE if BTC; FALSE if BCH

            // This BRAddress always satisfies BRAddressIsValid (given the corresponding
            // BRAddressParams).
            BRAddress addr;
        } btc;
        BREthereumAddress eth;
        struct {
            BRGenericWalletManager gwm;
            BRGenericAddress aid;
        } gen;
    } u;
    BRCryptoRef ref;
};

 private_extern BRCryptoAddress
 cryptoAddressCreateAsBTC (BRAddress btc,
                           BRCryptoBoolean isBTC);  // TRUE if BTC; FALSE if BCH

 private_extern BRCryptoAddress
 cryptoAddressCreateAsETH (BREthereumAddress eth);

 private_extern BRCryptoAddress
 cryptoAddressCreateAsGEN (BRGenericWalletManager gwm,
                           BRGenericAddress aid);

 private_extern BRCryptoBlockChainType
 cryptoAddressGetType (BRCryptoAddress address);

 private_extern BRAddress
 cryptoAddressAsBTC (BRCryptoAddress address,
                     BRCryptoBoolean *isBitcoinAddr);

 private_extern BREthereumAddress
 cryptoAddressAsETH (BRCryptoAddress address);

 private_extern BRGenericAddress
 cryptoAddressAsGEN (BRCryptoAddress address);


/**
 * Create an address from a NULL terminated BTC address string.
 *
 * The expected format differs depending on if the address is for mainnet vs
 * testnet, as well as legacy vs segwit. For example, a testnet segwit
 * address string will be of the form "tb1...".
 *
 * @return An address or NULL if the address string is invalid.
 */
private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsBTC (BRAddressParams params, const char *address);

/**
 * Create an addres from a NULL terminated BCH address string.
 *
 * @return An address or NULL if the address string is invalid.
 */
private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsBCH (BRAddressParams params, const char *bchAddress);

/**
 * Create an address from a NULL terminated ETH address string.
 *
 * An addresss string will be of the form "0x8fB4..." (with prefix)
 * or "8fB4..." (without prefix).
 *
 * @return An address or NULL if the address string is invalid.
 */
private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsETH (const char *address);

/**
 * ASSERT currently
 *
 * @return An address or NULL if the address string is invalid.
 */
private_extern BRCryptoAddress
cryptoAddressCreateFromStringAsGEN (const char *ethAddress);


#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAddressP_h */
