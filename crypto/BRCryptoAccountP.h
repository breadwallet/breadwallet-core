//
//  BRCryptoAccountP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoAccountP_h
#define BRCryptoAccountP_h

#include "BRCryptoAccount.h"

#include "support/BRBIP32Sequence.h"
#include "support/BRBIP39Mnemonic.h"
#include "support/BRKey.h"
#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BRCryptoAccountRecord {
    BRMasterPubKey btc;
    BREthereumAccount eth;
    BRGenericAccount xrp;
    // ...

    char *uids;
    uint64_t timestamp;
    BRCryptoRef ref;
};

/**
 * Given a phrase (A BIP-39 PaperKey) dervie the corresponding 'seed'.  This is used
 * exclusive to sign transactions (BTC ones for sure).
 *
 * @param phrase A BIP-32 Paper Key
 *
 * @return A UInt512 seed
 */
extern UInt512
cryptoAccountDeriveSeed (const char *phrase);


private_extern BREthereumAccount
cryptoAccountAsETH (BRCryptoAccount account);

private_extern BRGenericAccount
cryptoAccountAsGEN (BRCryptoAccount account,
                    const char *type);

private_extern const char *
cryptoAccountAddressAsETH (BRCryptoAccount account);

private_extern BRMasterPubKey
cryptoAccountAsBTC (BRCryptoAccount account);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoAccountP_h */
