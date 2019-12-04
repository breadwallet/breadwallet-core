//
//  BRCryptoNetwork.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoNetworkP.h"
#include "BRCryptoUnit.h"
#include "BRCryptoAddressP.h"

#include "BRCryptoPrivate.h"

#include "bitcoin/BRChainParams.h"
#include "bcash/BRBCashParams.h"
#include "ethereum/BREthereum.h"

/// MARK: - Network Fee

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoNetworkFee, cryptoNetworkFee)

extern BRCryptoNetworkFee
cryptoNetworkFeeCreate (uint64_t confirmationTimeInMilliseconds,
                        BRCryptoAmount pricePerCostFactor,
                        BRCryptoUnit   pricePerCostFactorUnit) {
    BRCryptoNetworkFee networkFee = calloc (1, sizeof (struct BRCryptoNetworkFeeRecord));

    networkFee->confirmationTimeInMilliseconds = confirmationTimeInMilliseconds;
    networkFee->pricePerCostFactor     = cryptoAmountTake (pricePerCostFactor);
    networkFee->pricePerCostFactorUnit = cryptoUnitTake (pricePerCostFactorUnit);
    networkFee->ref = CRYPTO_REF_ASSIGN(cryptoNetworkFeeRelease);

    return networkFee;
}

extern uint64_t
cryptoNetworkFeeGetConfirmationTimeInMilliseconds (BRCryptoNetworkFee networkFee) {
    return networkFee->confirmationTimeInMilliseconds;
}

extern BRCryptoAmount
cryptoNetworkFeeGetPricePerCostFactor (BRCryptoNetworkFee networkFee) {
    return cryptoAmountTake (networkFee->pricePerCostFactor);
}

extern BRCryptoUnit
cryptoNetworkFeeGetPricePerCostFactorUnit (BRCryptoNetworkFee networkFee) {
    return cryptoUnitTake (networkFee->pricePerCostFactorUnit);
}

extern BRCryptoBoolean
cryptoNetworkFeeEqual (BRCryptoNetworkFee nf1, BRCryptoNetworkFee nf2) {
    return AS_CRYPTO_BOOLEAN (nf1 == nf2 ||
                              (nf1->confirmationTimeInMilliseconds == nf2->confirmationTimeInMilliseconds) &&
                              CRYPTO_COMPARE_EQ == cryptoAmountCompare (nf1->pricePerCostFactor, nf2->pricePerCostFactor));
}

static void
cryptoNetworkFeeRelease (BRCryptoNetworkFee networkFee) {
    cryptoAmountGive (networkFee->pricePerCostFactor);
    cryptoUnitGive   (networkFee->pricePerCostFactorUnit);

    memset (networkFee, 0, sizeof(*networkFee));
    free (networkFee);
}

private_extern uint64_t
cryptoNetworkFeeAsBTC (BRCryptoNetworkFee networkFee) {
    BRCryptoBoolean overflow;
    uint64_t value = cryptoAmountGetIntegerRaw (networkFee->pricePerCostFactor, &overflow);
    assert (CRYPTO_FALSE == overflow);
    return value;
}

private_extern BREthereumGasPrice
cryptoNetworkFeeAsETH (BRCryptoNetworkFee networkFee) {
    UInt256 value = cryptoAmountGetValue (networkFee->pricePerCostFactor);
    return gasPriceCreate (etherCreate(value));
}

private_extern uint64_t
cryptoNetworkFeeAsGEN( BRCryptoNetworkFee networkFee) {
    BRCryptoBoolean overflow;
    uint64_t value = cryptoAmountGetIntegerRaw (networkFee->pricePerCostFactor, &overflow);
    assert (CRYPTO_FALSE == overflow);
    return value;
}

/// MARK: - Network

#define CRYPTO_NETWORK_DEFAULT_CURRENCY_ASSOCIATIONS        (2)
#define CRYPTO_NETWORK_DEFAULT_FEES                         (3)
#define CRYPTO_NETWORK_DEFAULT_NETWORKS                     (5)

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoNetwork, cryptoNetwork)

static BRCryptoNetwork
cryptoNetworkCreate (const char *uids,
                     const char *name) {
    BRCryptoNetwork network = malloc (sizeof (struct BRCryptoNetworkRecord));

    network->uids = strdup (uids);
    network->name = strdup (name);
    network->currency = NULL;
    network->height = 0;
    array_new (network->associations, CRYPTO_NETWORK_DEFAULT_CURRENCY_ASSOCIATIONS);
    array_new (network->fees, CRYPTO_NETWORK_DEFAULT_FEES);

    network->ref = CRYPTO_REF_ASSIGN(cryptoNetworkRelease);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&network->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return network;
}

// TODO: Remove forkId; derivable from BRChainParams (after CORE-284)
private_extern BRCryptoNetwork
cryptoNetworkCreateAsBTC (const char *uids,
                          const char *name,
                          uint8_t forkId,
                          const BRChainParams *params) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name);
    network->type = BLOCK_CHAIN_TYPE_BTC;
    network->u.btc.forkId = forkId;
    network->u.btc.params = params;

    return network;
}

private_extern BRCryptoNetwork
cryptoNetworkCreateAsETH (const char *uids,
                          const char *name,
                          uint32_t chainId,
                          BREthereumNetwork net) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name);
    network->type = BLOCK_CHAIN_TYPE_ETH;
    network->u.eth.chainId = chainId;
    network->u.eth.net = net;

    return network;
}

private_extern BRCryptoNetwork
cryptoNetworkCreateAsGEN (const char *uids,
                          const char *name,
                          uint8_t isMainnet) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name);
    network->type = BLOCK_CHAIN_TYPE_GEN;
    network->u.gen.mainnet = isMainnet;
    return network;
}

static void
cryptoNetworkRelease (BRCryptoNetwork network) {
    for (size_t index = 0; index < array_count (network->associations); index++) {
        BRCryptoCurrencyAssociation *association = &network->associations[index];
        cryptoCurrencyGive (association->currency);
        cryptoUnitGive (association->baseUnit);
        cryptoUnitGive (association->defaultUnit);
        cryptoUnitGiveAll (association->units);
        array_free (association->units);
    }
    array_free (network->associations);

    for (size_t index = 0; index < array_count (network->fees); index++) {
        cryptoNetworkFeeGive (network->fees[index]);
    }
    array_free (network->fees);

    // TBD
    switch (network->type){
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }

    free (network->name);
    free (network->uids);
    if (NULL != network->currency) cryptoCurrencyGive (network->currency);
    pthread_mutex_destroy (&network->lock);

    memset (network, 0, sizeof(*network));
    free (network);
}

extern BRCryptoBlockChainType
cryptoNetworkGetType (BRCryptoNetwork network) {
    return network->type;
}

extern const char *
cryptoNetworkGetUids (BRCryptoNetwork network) {
    return network->uids;
}

extern const char *
cryptoNetworkGetName (BRCryptoNetwork network) {
    return network->name;
}

extern BRCryptoBoolean
cryptoNetworkIsMainnet (BRCryptoNetwork network) {
    switch (network->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return AS_CRYPTO_BOOLEAN (network->u.btc.params == BRMainNetParams ||
                                      network->u.btc.params == BRBCashParams);
        case BLOCK_CHAIN_TYPE_ETH:
            return AS_CRYPTO_BOOLEAN (network->u.eth.net == ethereumMainnet);
        case BLOCK_CHAIN_TYPE_GEN:
            return AS_CRYPTO_BOOLEAN (network->u.gen.mainnet);
    }
}

extern BRCryptoBlockChainHeight
cryptoNetworkGetHeight (BRCryptoNetwork network) {
    return network->height;
}

extern void
cryptoNetworkSetHeight (BRCryptoNetwork network,
                        BRCryptoBlockChainHeight height) {
    network->height = height;
}

extern uint32_t
cryptoNetworkGetConfirmationsUntilFinal (BRCryptoNetwork network) {
    return network->confirmationsUntilFinal;
}

extern void
cryptoNetworkSetConfirmationsUntilFinal (BRCryptoNetwork network,
                                         uint32_t confirmationsUntilFinal) {
    network->confirmationsUntilFinal = confirmationsUntilFinal;
}

extern BRCryptoCurrency
cryptoNetworkGetCurrency (BRCryptoNetwork network) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrency currency = cryptoCurrencyTake (network->currency);
    pthread_mutex_unlock (&network->lock);
    return currency;
}

extern const char *
cryptoNetworkGetCurrencyCode (BRCryptoNetwork network) {
    return cryptoCurrencyGetCode (network->currency);
}

extern void
cryptoNetworkSetCurrency (BRCryptoNetwork network,
                          BRCryptoCurrency newCurrency) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrency currency = network->currency;
    network->currency = cryptoCurrencyTake(newCurrency);
    pthread_mutex_unlock (&network->lock);

    // drop reference outside of lock to avoid potential case where release function runs
    if (NULL != currency) cryptoCurrencyGive (currency);
}

extern size_t
cryptoNetworkGetCurrencyCount (BRCryptoNetwork network) {
    pthread_mutex_lock (&network->lock);
    size_t count = array_count (network->associations);
    pthread_mutex_unlock (&network->lock);
    return count;
}

extern BRCryptoCurrency
cryptoNetworkGetCurrencyAt (BRCryptoNetwork network,
                            size_t index) {
    pthread_mutex_lock (&network->lock);
    assert (index < array_count(network->associations));
    BRCryptoCurrency currency = cryptoCurrencyTake (network->associations[index].currency);
    pthread_mutex_unlock (&network->lock);
    return currency;
}

extern BRCryptoBoolean
cryptoNetworkHasCurrency (BRCryptoNetwork network,
                          BRCryptoCurrency currency) {
    BRCryptoBoolean r = CRYPTO_FALSE;
    pthread_mutex_lock (&network->lock);
    for (size_t index = 0; index < array_count(network->associations) && CRYPTO_FALSE == r; index++) {
        r = cryptoCurrencyIsIdentical (network->associations[index].currency, currency);
    }
    pthread_mutex_unlock (&network->lock);
    return r;
}

extern BRCryptoCurrency
cryptoNetworkGetCurrencyForCode (BRCryptoNetwork network,
                                   const char *code) {
    BRCryptoCurrency currency = NULL;
    pthread_mutex_lock (&network->lock);
    for (size_t index = 0; index < array_count(network->associations); index++) {
        if (0 == strcmp (code, cryptoCurrencyGetCode (network->associations[index].currency))) {
            currency = cryptoCurrencyTake (network->associations[index].currency);
            break;
        }
    }
    pthread_mutex_unlock (&network->lock);
    return currency;
}

private_extern BRCryptoCurrency
cryptoNetworkGetCurrencyforTokenETH (BRCryptoNetwork network,
                                     BREthereumToken token) {
    BRCryptoCurrency tokenCurrency = NULL;
    pthread_mutex_lock (&network->lock);
    for (size_t index = 0; index < array_count(network->associations); index++) {
        BRCryptoCurrency currency = network->associations[index].currency;
        const char *address = cryptoCurrencyGetIssuer (currency);

        if (NULL != address && ETHEREUM_BOOLEAN_IS_TRUE (tokenHasAddress (token, address))) {
            tokenCurrency = cryptoCurrencyTake (currency);
            break;
        }
    }
    pthread_mutex_unlock (&network->lock);
    return tokenCurrency;
}

static BRCryptoCurrencyAssociation *
cryptoNetworkLookupCurrency (BRCryptoNetwork network,
                             BRCryptoCurrency currency) {
    // lock is not held for this static method; caller must hold it
    for (size_t index = 0; index < array_count(network->associations); index++) {
        if (currency == network->associations[index].currency) {
            return &network->associations[index];
        }
    }
    return NULL;
}

extern BRCryptoUnit
cryptoNetworkGetUnitAsBase (BRCryptoNetwork network,
                            BRCryptoCurrency currency) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrencyAssociation *association = cryptoNetworkLookupCurrency (network, currency);
    BRCryptoUnit unit = NULL == association ? NULL : cryptoUnitTake (association->baseUnit);
    pthread_mutex_unlock (&network->lock);
    return unit;
}

extern BRCryptoUnit
cryptoNetworkGetUnitAsDefault (BRCryptoNetwork network,
                               BRCryptoCurrency currency) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrencyAssociation *association = cryptoNetworkLookupCurrency (network, currency);
    BRCryptoUnit unit = NULL == association ? NULL : cryptoUnitTake (association->defaultUnit);
    pthread_mutex_unlock (&network->lock);
    return unit;
}

extern size_t
cryptoNetworkGetUnitCount (BRCryptoNetwork network,
                           BRCryptoCurrency currency) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrencyAssociation *association = cryptoNetworkLookupCurrency (network, currency);
    size_t count = ((NULL == association || NULL == association->units)
                    ? 0
                    : array_count (association->units));
    pthread_mutex_unlock (&network->lock);
    return count;
}

extern BRCryptoUnit
cryptoNetworkGetUnitAt (BRCryptoNetwork network,
                        BRCryptoCurrency currency,
                        size_t index) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrencyAssociation *association = cryptoNetworkLookupCurrency (network, currency);
    BRCryptoUnit unit = ((NULL == association || NULL == association->units || index >= array_count(association->units))
                         ? NULL
                         : cryptoUnitTake (association->units[index]));
    pthread_mutex_unlock (&network->lock);
    return unit;
}

extern void
cryptoNetworkAddCurrency (BRCryptoNetwork network,
                          BRCryptoCurrency currency,
                          BRCryptoUnit baseUnit,
                          BRCryptoUnit defaultUnit) {
    BRCryptoCurrencyAssociation association = {
        cryptoCurrencyTake (currency),
        cryptoUnitTake (baseUnit),
        cryptoUnitTake (defaultUnit),
        NULL
    };

    pthread_mutex_lock (&network->lock);
    array_new (association.units, 2);
    array_add (network->associations, association);
    pthread_mutex_unlock (&network->lock);
}

extern void
cryptoNetworkAddCurrencyUnit (BRCryptoNetwork network,
                              BRCryptoCurrency currency,
                              BRCryptoUnit unit) {
    pthread_mutex_lock (&network->lock);
    BRCryptoCurrencyAssociation *association = cryptoNetworkLookupCurrency (network, currency);
    if (NULL != association) array_add (association->units, cryptoUnitTake (unit));
    pthread_mutex_unlock (&network->lock);
}

extern void
cryptoNetworkAddNetworkFee (BRCryptoNetwork network,
                            BRCryptoNetworkFee fee) {
    pthread_mutex_lock (&network->lock);
    array_add (network->fees, cryptoNetworkFeeTake (fee));
    pthread_mutex_unlock (&network->lock);
}

extern void
cryptoNetworkSetNetworkFees (BRCryptoNetwork network,
                             const BRCryptoNetworkFee *fees,
                             size_t count) {
    assert (0 != count);
    pthread_mutex_lock (&network->lock);
    array_apply (network->fees, cryptoNetworkFeeGive);
    array_clear (network->fees);
    for (size_t idx = 0; idx < count; idx++) {
        array_add (network->fees, cryptoNetworkFeeTake (fees[idx]));
    }
    pthread_mutex_unlock (&network->lock);
}

extern BRCryptoNetworkFee *
cryptoNetworkGetNetworkFees (BRCryptoNetwork network,
                             size_t *count) {
    pthread_mutex_lock (&network->lock);
    *count = array_count (network->fees);
    BRCryptoNetworkFee *fees = NULL;
    if (0 != *count) {
        fees = calloc (*count, sizeof(BRCryptoNetworkFee));
        for (size_t index = 0; index < *count; index++) {
            fees[index] = cryptoNetworkFeeTake(network->fees[index]);
        }
    }
    pthread_mutex_unlock (&network->lock);
    return fees;
}

extern BRCryptoAddress
cryptoNetworkCreateAddressFromString (BRCryptoNetwork network,
                                      const char *string) {
    switch (network->type) {

        case BLOCK_CHAIN_TYPE_BTC:
            return (BRChainParamsIsBitcoin (network->u.btc.params)
                    ? cryptoAddressCreateFromStringAsBTC (network->u.btc.params->addrParams, string)
                    : cryptoAddressCreateFromStringAsBCH (network->u.btc.params->addrParams, string));

        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAddressCreateFromStringAsETH (string);

        case BLOCK_CHAIN_TYPE_GEN:
            return cryptoAddressCreateFromStringAsGEN (string);
    }
}

// TODO(discuss): Is it safe to give out this pointer?
private_extern BREthereumNetwork
cryptoNetworkAsETH (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_ETH == network->type);
    return network->u.eth.net;
}

// TODO(discuss): Is it safe to give out this pointer?
private_extern const BRChainParams *
cryptoNetworkAsBTC (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_BTC == network->type);
    return network->u.btc.params;
}

private_extern void *
cryptoNetworkAsGEN (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_GEN == network->type);
    return NULL;
}

private_extern BRCryptoBlockChainType
cryptoNetworkGetBlockChainType (BRCryptoNetwork network) {
    return network->type;
}
