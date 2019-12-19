//
//  BRCryptoFeeBasis.h
//  Core
//
//  Created by Ed Gamble on 5/15/19.
//  Copyright © 2019 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoFeeBasis_h
#define BRCryptoFeeBasis_h

#include "BRCryptoBase.h"
#include "BRCryptoCurrency.h"
#include "BRCryptoAmount.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoFeeBasisRecord *BRCryptoFeeBasis;

    extern BRCryptoAmount
    cryptoFeeBasisGetPricePerCostFactor (BRCryptoFeeBasis feeBasis);

    extern BRCryptoUnit
    cryptoFeeBasisGetPricePerCostFactorUnit (BRCryptoFeeBasis feeBasis);

    extern double
    cryptoFeeBasisGetCostFactor (BRCryptoFeeBasis feeBasis);

    extern BRCryptoAmount
    cryptoFeeBasisGetFee (BRCryptoFeeBasis feeBasis);

    extern BRCryptoBoolean
    cryptoFeeBasisIsIdentical (BRCryptoFeeBasis feeBasis1,
                               BRCryptoFeeBasis feeBasis2);
    
    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoFeeBasis, cryptoFeeBasis);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoFeeBasis_h */
