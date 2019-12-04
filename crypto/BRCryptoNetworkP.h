//
//  BRCryptoNetworkP.h
//  BRCore
//
//  Created by Ed Gamble on 11/22/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoNetworkP_h
#define BRCryptoNetworkP_h

#include <pthread.h>

#include "BRCryptoBase.h"
#include "BRCryptoNetwork.h"

#include "bitcoin/BRChainParams.h"
#include "bcash/BRBCashParams.h"
#include "ethereum/BREthereum.h"

#ifdef __cplusplus
extern "C" {
#endif

/// MARK: - Network Fee

struct BRCryptoNetworkFeeRecord {
    uint64_t confirmationTimeInMilliseconds;
    BRCryptoAmount pricePerCostFactor;
    BRCryptoUnit   pricePerCostFactorUnit;  // Until in BRCryptoAmount
    BRCryptoRef ref;
};

private_extern BRCryptoNetworkFee
cryptoNetworkFeeCreate (uint64_t confirmationTimeInMilliseconds,
                        BRCryptoAmount pricePerCostFactor,
                        BRCryptoUnit   pricePerCostFactorUnit);

private_extern uint64_t
cryptoNetworkFeeAsBTC (BRCryptoNetworkFee networkFee);

private_extern BREthereumGasPrice
cryptoNetworkFeeAsETH (BRCryptoNetworkFee networkFee);

private_extern uint64_t
cryptoNetworkFeeAsGEN( BRCryptoNetworkFee networkFee);

/// MARK: - Currency Association

typedef struct {
    BRCryptoCurrency currency;
    BRCryptoUnit baseUnit;
    BRCryptoUnit defaultUnit;
    BRArrayOf(BRCryptoUnit) units;
} BRCryptoCurrencyAssociation;

/// MARK: - Network

struct BRCryptoNetworkRecord {
    pthread_mutex_t lock;

    char *uids;
    char *name;
    BRCryptoBlockChainHeight height;
    BRCryptoCurrency currency;
    BRArrayOf(BRCryptoCurrencyAssociation) associations;
    BRArrayOf(BRCryptoNetworkFee) fees;

    uint32_t confirmationsUntilFinal;

    BRCryptoBlockChainType type;
    union {
        struct {
            uint8_t forkId;
            const BRChainParams *params;
        } btc;

        struct {
            uint32_t chainId;
            BREthereumNetwork net;
        } eth;

        struct {
            // TODO: TBD
            uint8_t mainnet;
        } gen;
    } u;
    BRCryptoRef ref;
};


 private_extern void
 cryptoNetworkAnnounce (BRCryptoNetwork network);

 private_extern void
 cryptoNetworkSetHeight (BRCryptoNetwork network,
                         BRCryptoBlockChainHeight height);

 private_extern void
 cryptoNetworkSetConfirmationsUntilFinal (BRCryptoNetwork network,
                                          uint32_t confirmationsUntilFinal);

 private_extern void
 cryptoNetworkSetCurrency (BRCryptoNetwork network,
                           BRCryptoCurrency currency);

 private_extern void
 cryptoNetworkAddCurrency (BRCryptoNetwork network,
                           BRCryptoCurrency currency,
                           BRCryptoUnit baseUnit,
                           BRCryptoUnit defaultUnit);

 private_extern void
 cryptoNetworkAddCurrencyUnit (BRCryptoNetwork network,
                               BRCryptoCurrency currency,
                               BRCryptoUnit unit);

 private_extern void
 cryptoNetworkAddNetworkFee (BRCryptoNetwork network,
                             BRCryptoNetworkFee fee);

 private_extern void
 cryptoNetworkSetNetworkFees (BRCryptoNetwork network,
                              const BRCryptoNetworkFee *fees,
                              size_t count);

 private_extern BREthereumNetwork
 cryptoNetworkAsETH (BRCryptoNetwork network);

 private_extern const BRChainParams *
 cryptoNetworkAsBTC (BRCryptoNetwork network);

 private_extern void *
 cryptoNetworkAsGEN (BRCryptoNetwork network);

 private_extern BRCryptoNetwork
 cryptoNetworkCreateAsBTC (const char *uids,
                           const char *name,
                           uint8_t forkId,
                           const BRChainParams *params);

 private_extern BRCryptoNetwork
 cryptoNetworkCreateAsETH (const char *uids,
                           const char *name,
                           uint32_t chainId,
                           BREthereumNetwork net);

 private_extern BRCryptoNetwork
 cryptoNetworkCreateAsGEN (const char *uids,
                           const char *name,
                           uint8_t isMainnet);

 private_extern BRCryptoBlockChainType
 cryptoNetworkGetBlockChainType (BRCryptoNetwork network);

 private_extern BRCryptoCurrency
 cryptoNetworkGetCurrencyforTokenETH (BRCryptoNetwork network,
                                      BREthereumToken token);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoNetworkP_h */
