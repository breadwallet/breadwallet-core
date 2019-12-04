//
//  BRCryptoFeeBasisP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

#ifndef BRCryptoFeeBasisP_h
#define BRCryptoFeeBasisP_h

#include "BRCryptoFeeBasis.h"

#include "ethereum/BREthereum.h"
#include "generic/BRGeneric.h"

#ifdef __cplusplus
extern "C" {
#endif

struct BRCryptoFeeBasisRecord {
    BRCryptoBlockChainType type;
    union {
        struct {
            uint32_t feePerKB;
            uint32_t sizeInByte;
        } btc;
        BREthereumFeeBasis eth;
        struct {
            BRGenericWalletManager gwm;
            BRGenericFeeBasis bid;
        } gen;
    } u;
    BRCryptoUnit unit;
    BRCryptoRef ref;
};

 private_extern uint64_t
 cryptoFeeBasisAsBTC (BRCryptoFeeBasis feeBasis);

 private_extern BREthereumFeeBasis
 cryptoFeeBasisAsETH (BRCryptoFeeBasis feeBasis);

 private_extern BRGenericFeeBasis
 cryptoFeeBasisAsGEN (BRCryptoFeeBasis feeBasis);

 private_extern BRCryptoFeeBasis
 cryptoFeeBasisCreateAsBTC (BRCryptoUnit unit,
                            uint32_t feePerKB,
                            uint32_t sizeInByte);

 private_extern BRCryptoFeeBasis
 cryptoFeeBasisCreateAsETH (BRCryptoUnit unit,
                            BREthereumGas gas,
                            BREthereumGasPrice gasPrice);

 private_extern BRCryptoFeeBasis
 cryptoFeeBasisCreateAsGEN (BRCryptoUnit unit,
                            BRGenericWalletManager gwm,
                            OwnershipGiven BRGenericFeeBasis bid);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoFeeBasisP_h */
