//
//  BRCryptoFeeBasis.c
//  Core
//
//  Created by Ed Gamble on 5/15/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#include "BRCryptoFeeBasis.h"
#include "ethereum/ewm/BREthereumBase.h"

static void
cryptoFeeBasisRelease (BRCryptoFeeBasis feeBasis);

struct BRCryptoFeeBasisRecord {
    BRCryptoBlockChainType type;
    union {
        uint64_t btc; // feePerKB
        BREthereumFeeBasis eth;
    } u;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoFeeBasis, cryptoFeeBasis)

static BRCryptoFeeBasis
cryptoFeeBasisCreateInternal (BRCryptoBlockChainType type) {
    BRCryptoFeeBasis feeBasis = malloc (sizeof (struct BRCryptoFeeBasisRecord));

    feeBasis->type = type;
    feeBasis->ref  = CRYPTO_REF_ASSIGN (cryptoFeeBasisRelease);

    return feeBasis;
}

private_extern BRCryptoFeeBasis
cryptoFeeBasisCreateAsBTC (uint64_t feePerKB) {
    BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateInternal (BLOCK_CHAIN_TYPE_BTC);
    feeBasis->u.btc = feePerKB;

    return feeBasis;
}

private_extern BRCryptoFeeBasis
cryptoFeeBasisCreateAsETH (BREthereumGas gas,
                           BREthereumGasPrice gasPrice) {
    BRCryptoFeeBasis feeBasis = cryptoFeeBasisCreateInternal (BLOCK_CHAIN_TYPE_ETH);
    feeBasis->u.eth = (BREthereumFeeBasis) {
        FEE_BASIS_GAS,
        { .gas = { gas, gasPrice }}
    };

    return feeBasis;
}

static void
cryptoFeeBasisRelease (BRCryptoFeeBasis feeBasis) {
    free (feeBasis);
}

extern BRCryptoBlockChainType
cryptoFeeBasisGetType (BRCryptoFeeBasis feeBasis) {
    return feeBasis->type;
}

private_extern uint64_t
cryptoFeeBasisAsBTC (BRCryptoFeeBasis feeBasis) {
    assert (BLOCK_CHAIN_TYPE_BTC == feeBasis->type);
    return feeBasis->u.btc;
}

private_extern BREthereumFeeBasis
cryptoFeeBasisAsETH (BRCryptoFeeBasis feeBasis) {
    assert (BLOCK_CHAIN_TYPE_ETH == feeBasis->type);
    return feeBasis->u.eth;
}
