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
#include "BRCryptoAmountP.h"
#include "BRCryptoAccountP.h"

#include "bitcoin/BRChainParams.h"
#include "bcash/BRBCashParams.h"
#include "ethereum/BREthereum.h"

#include <stdbool.h>

private_extern BRArrayOf(BRCryptoUnit)
cryptoUnitGiveAll (BRArrayOf(BRCryptoUnit) units);

/// MARK: - Network Canonical Type

extern const char *
cryptoNetworkCanonicalTypeString (BRCryptoNetworkCanonicalType type) {
    static const char *strings[NUMBER_OF_NETWORK_TYPES] = {
        "CRYPTO_NETWORK_TYPE_BTC",
        "CRYPTO_NETWORK_TYPE_BCH",
        "CRYPTO_NETWORK_TYPE_ETH",
        "CRYPTO_NETWORK_TYPE_XRP"
        // "Hedera"
        // "Stellar"
    };
    assert (type < NUMBER_OF_NETWORK_TYPES);
    return strings[type];
}

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
                     const char *name,
                     BRCryptoNetworkCanonicalType canonicalType) {
    cryptoAccountInstall();

    BRCryptoNetwork network = calloc (1, sizeof (struct BRCryptoNetworkRecord));

    network->canonicalType = canonicalType;
    network->uids = strdup (uids);
    network->name = strdup (name);
    network->currency = NULL;
    network->height = 0;
    array_new (network->associations, CRYPTO_NETWORK_DEFAULT_CURRENCY_ASSOCIATIONS);
    array_new (network->fees, CRYPTO_NETWORK_DEFAULT_FEES);

    network->addressSchemes = NULL;
    network->syncModes = NULL;

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
static BRCryptoNetwork
cryptoNetworkCreateAsBTC (const char *uids,
                          const char *name,
                          const BRChainParams *params) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name, CRYPTO_NETWORK_TYPE_BTC);
    network->type = BLOCK_CHAIN_TYPE_BTC;
    network->u.btc = params;

    return network;
}

static BRCryptoNetwork
cryptoNetworkCreateAsBCH (const char *uids,
                          const char *name,
                          const BRChainParams *params) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name, CRYPTO_NETWORK_TYPE_BCH);
    network->type = BLOCK_CHAIN_TYPE_BTC;
    network->u.btc = params;

    return network;
}

static BRCryptoNetwork
cryptoNetworkCreateAsETH (const char *uids,
                          const char *name,
                          BREthereumNetwork net) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name, CRYPTO_NETWORK_TYPE_ETH);
    network->type = BLOCK_CHAIN_TYPE_ETH;
    network->u.eth = net;

    return network;
}

static BRCryptoNetwork
cryptoNetworkCreateAsGEN (const char *uids,
                          const char *name,
                          const char *type,
                          uint8_t isMainnet,
                          BRCryptoNetworkCanonicalType canonicalType) {
    BRCryptoNetwork network = cryptoNetworkCreate (uids, name, canonicalType);
    network->type = BLOCK_CHAIN_TYPE_GEN;
    network->u.gen = genNetworkCreate(type, isMainnet);
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

    if (network->addressSchemes) array_free (network->addressSchemes);
    if (network->syncModes)      array_free (network->syncModes);
        
    // TBD
    switch (network->type){
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            genNetworkRelease (network->u.gen);
            break;
    }

    free (network->name);
    free (network->uids);
    if (NULL != network->currency) cryptoCurrencyGive (network->currency);
    pthread_mutex_destroy (&network->lock);

    memset (network, 0, sizeof(*network));
    free (network);
}

extern BRCryptoNetworkCanonicalType
cryptoNetworkGetCanonicalType (BRCryptoNetwork network) {
    return network->canonicalType;
}

private_extern BRCryptoBlockChainType
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
            return AS_CRYPTO_BOOLEAN (network->u.btc == BRMainNetParams ||
                                      network->u.btc == BRBCashParams);
        case BLOCK_CHAIN_TYPE_ETH:
            return AS_CRYPTO_BOOLEAN (network->u.eth == ethereumMainnet);
        case BLOCK_CHAIN_TYPE_GEN:
            return AS_CRYPTO_BOOLEAN (genNetworkIsMainnet (network->u.gen));
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

extern BRCryptoCurrency
cryptoNetworkGetCurrencyForUids (BRCryptoNetwork network,
                                   const char *uids) {
    BRCryptoCurrency currency = NULL;
    pthread_mutex_lock (&network->lock);
    for (size_t index = 0; index < array_count(network->associations); index++) {
        if (0 == strcmp (uids, cryptoCurrencyGetUids (network->associations[index].currency))) {
            currency = cryptoCurrencyTake (network->associations[index].currency);
            break;
        }
    }
    pthread_mutex_unlock (&network->lock);
    return currency;
}

extern BRCryptoCurrency
cryptoNetworkGetCurrencyForIssuer (BRCryptoNetwork network,
                                   const char *issuer) {
    BRCryptoCurrency currency = NULL;
    pthread_mutex_lock (&network->lock);
    for (size_t index = 0; index < array_count(network->associations); index++) {
        const char *i = cryptoCurrencyGetIssuer(network->associations[index].currency);
        if (NULL != i && 0 == strcasecmp (issuer, i)) {
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
        if (CRYPTO_TRUE == cryptoCurrencyIsIdentical (currency, network->associations[index].currency)) {
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

// MARK: - Address Scheme

extern BRCryptoAddressScheme
cryptoNetworkGetDefaultAddressScheme (BRCryptoNetwork network) {
    assert (NULL != network->addressSchemes);
    return network->defaultAddressScheme;
}

static void
cryptoNetworkAddSupportedAddressScheme (BRCryptoNetwork network,
                                        BRCryptoAddressScheme scheme) {
    if (NULL == network->addressSchemes) array_new (network->addressSchemes, NUMBER_OF_ADDRESS_SCHEMES);
    array_add (network->addressSchemes, scheme);
}

extern const BRCryptoAddressScheme *
cryptoNetworkGetSupportedAddressSchemes (BRCryptoNetwork network,
                                         BRCryptoCount *count) {
    assert (NULL != network->addressSchemes);
    assert (NULL != count);
    *count = array_count(network->addressSchemes);
    return network->addressSchemes;
}

extern BRCryptoBoolean
cryptoNetworkSupportsAddressScheme (BRCryptoNetwork network,
                                    BRCryptoAddressScheme scheme) {
    assert (NULL != network->addressSchemes);
    for (size_t index = 0; index < array_count (network->addressSchemes); index++)
        if (scheme == network->addressSchemes[index])
            return CRYPTO_TRUE;
    return CRYPTO_FALSE;
}

// MARK: - Sync Mode

extern BRCryptoSyncMode
cryptoNetworkGetDefaultSyncMode (BRCryptoNetwork network) {
    assert (NULL != network->syncModes);
    return network->defaultSyncMode;
}

static void
cryptoNetworkAddSupportedSyncMode (BRCryptoNetwork network,
                                   BRCryptoSyncMode scheme) {
    if (NULL == network->syncModes) array_new (network->syncModes, NUMBER_OF_SYNC_MODES);
    array_add (network->syncModes, scheme);
}

extern const BRCryptoSyncMode *
cryptoNetworkGetSupportedSyncModes (BRCryptoNetwork network,
                                    BRCryptoCount *count) {
    assert (NULL != network->syncModes);
    assert (NULL != count);
    *count = array_count(network->syncModes);
    return network->syncModes;
}

extern BRCryptoBoolean
cryptoNetworkSupportsSyncMode (BRCryptoNetwork network,
                               BRCryptoSyncMode mode) {
    assert (NULL != network->syncModes);
    for (size_t index = 0; index < array_count (network->syncModes); index++)
        if (mode == network->syncModes[index])
            return CRYPTO_TRUE;
    return CRYPTO_FALSE;
}

extern BRCryptoBoolean
cryptoNetworkRequiresMigration (BRCryptoNetwork network) {
    return (CRYPTO_NETWORK_TYPE_BTC == network->canonicalType ||
            CRYPTO_NETWORK_TYPE_BCH == network->canonicalType);
}

// TODO(discuss): Is it safe to give out this pointer?
private_extern const BRChainParams *
cryptoNetworkAsBTC (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_BTC == network->type);
    return network->u.btc;
}

// TODO(discuss): Is it safe to give out this pointer?
private_extern BREthereumNetwork
cryptoNetworkAsETH (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_ETH == network->type);
    return network->u.eth;
}

// TODO(discuss): Is it safe to give out this pointer?
private_extern BRGenericNetwork
cryptoNetworkAsGEN (BRCryptoNetwork network) {
    assert (BLOCK_CHAIN_TYPE_GEN == network->type);
    return network->u.gen;
}

private_extern BRCryptoBlockChainType
cryptoNetworkGetBlockChainType (BRCryptoNetwork network) {
    return network->type;
}

// MARK: - Network Defaults

static BRCryptoNetwork
cryptoNetworkCreateBuiltin (const char *symbol,
                            const char *uids,
                            const char *name) {
    BRCryptoNetwork network = NULL; // cryptoNetworkCreate (uids, name);
    // BTC, BCH, ETH, GEN

    if      (0 == strcmp ("btcMainnet", symbol))
        network = cryptoNetworkCreateAsBTC (uids, name, BRMainNetParams);
    else if (0 == strcmp ("btcTestnet", symbol))
        network = cryptoNetworkCreateAsBTC (uids, name, BRTestNetParams);
    else if (0 == strcmp ("bchMainnet", symbol))
        network = cryptoNetworkCreateAsBCH (uids, name, BRBCashParams);
    else if (0 == strcmp ("bchTestnet", symbol))
        network = cryptoNetworkCreateAsBCH (uids, name, BRBCashTestNetParams);
    else if (0 == strcmp ("ethMainnet", symbol))
        network = cryptoNetworkCreateAsETH (uids, name, ethereumMainnet);
    else if (0 == strcmp ("ethRopsten", symbol))
        network = cryptoNetworkCreateAsETH (uids, name, ethereumTestnet);
    else if (0 == strcmp ("ethRinkeby", symbol))
        network = cryptoNetworkCreateAsETH (uids, name, ethereumRinkeby);
    else if (0 == strcmp ("xrpMainnet", symbol))
        network = cryptoNetworkCreateAsGEN (uids, name, GEN_NETWORK_TYPE_XRP, 1, CRYPTO_NETWORK_TYPE_XRP);
    else if (0 == strcmp ("xrpTestnet", symbol))
        network = cryptoNetworkCreateAsGEN (uids, name, GEN_NETWORK_TYPE_XRP, 0, CRYPTO_NETWORK_TYPE_XRP);
//    else if (0 == strcmp ("hbarMainnet", symbol))
//        network = cryptoNetworkCreateAsGEN (uids, name, GEN_NETWORK_TYPE_HBAR, 1, CRYPTO_NETWORK_TYPE_HBAR);
//    else if (0 == strcmp ("xlmMainnet", symbol))
//        network = cryptoNetworkCreateAsGEN (uids, name, GEN_NETWORK_TYPE_Xlm, 1, CRYPTO_NETWORK_TYPE_XLM);
    // ...

    assert (NULL != network);
    return network;
}

extern BRCryptoNetwork *
cryptoNetworkInstallBuiltins (BRCryptoCount *networksCount) {
    cryptoAccountInstall();

    // Network Specification
    struct NetworkSpecification {
        char *symbol;
        char *networkId;
        char *name;
        char *network;
        bool isMainnet;
        uint64_t height;
        uint32_t confirmations;
    } networkSpecifications[] = {
#define DEFINE_NETWORK(symbol, networkId, name, network, isMainnet, height, confirmations) \
{ #symbol, networkId, name, network, isMainnet, height, confirmations },
#include "BRCryptoConfig.h"
//        { NULL }
    };
    size_t NUMBER_OF_NETWORKS = sizeof (networkSpecifications) / sizeof (struct NetworkSpecification);

    // Network Fee Specification
    struct NetworkFeeSpecification {
        char *networkId;
        char *amount;
        char *tier;
        uint32_t confirmationTimeInMilliseconds;
    } networkFeeSpecifications[] = {
#define DEFINE_NETWORK_FEE_ESTIMATE(networkId, amount, tier, confirmationTimeInMilliseconds)\
{ networkId, amount, tier, confirmationTimeInMilliseconds },
#include "BRCryptoConfig.h"
    };
    size_t NUMBER_OF_FEES = sizeof (networkFeeSpecifications) / sizeof (struct NetworkFeeSpecification);

    // Currency Specification
    struct CurrencySpecification {
        char *networkId;
        char *currencyId;
        char *name;
        char *code;
        char *type;
        char *address;
        bool verified;
    } currencySpecifications[] = {
#define DEFINE_CURRENCY(networkId, currencyId, name, code, type, address, verified) \
{ networkId, currencyId, name, code, type, address, verified },
#include "BRCryptoConfig.h"
    };
    size_t NUMBER_OF_CURRENCIES = sizeof (currencySpecifications) / sizeof(struct CurrencySpecification);

    // Unit Specification
    struct UnitSpecification {
        char *currencyId;
        char *name;
        char *code;
        uint32_t decimals;
        char *symbol;
    } unitSpecifications[] = {
#define DEFINE_UNIT(currencyId, name, code, decimals, symbol) \
{ currencyId, name, code, decimals, symbol },
#include "BRCryptoConfig.h"
    };
    size_t NUMBER_OF_UNITS = sizeof (unitSpecifications) / sizeof (struct UnitSpecification);

    // Address Schemes
    struct AddressSchemeSpecification {
        char *networkId;
        BRCryptoAddressScheme defaultScheme;
        BRCryptoAddressScheme schemes[NUMBER_OF_ADDRESS_SCHEMES];
        size_t numberOfSchemes;
    } addressSchemeSpecs[] = {
#define VAR_SCHEMES_COUNT(...)    (sizeof((BRCryptoAddressScheme[]){__VA_ARGS__})/sizeof(BRCryptoAddressScheme))
#define DEFINE_ADDRESS_SCHEMES(networkId, defaultScheme, otherSchemes...) \
{ networkId, defaultScheme, { defaultScheme, otherSchemes }, 1 + VAR_SCHEMES_COUNT(otherSchemes) },
#include "BRCryptoConfig.h"
    };
    size_t NUMBER_OF_SCHEMES = sizeof (addressSchemeSpecs) / sizeof (struct AddressSchemeSpecification);

    // Sync Modes
    struct SyncModeSpecification {
        char *networkId;
        BRCryptoSyncMode defaultMode;
        BRCryptoSyncMode modes[NUMBER_OF_SYNC_MODES];
        size_t numberOfModes;
    } modeSpecs[] = {
#define VAR_MODES_COUNT(...)    (sizeof((BRCryptoSyncMode[]){__VA_ARGS__})/sizeof(BRCryptoSyncMode))
#define DEFINE_MODES(networkId, defaultMode, otherModes...) \
{ networkId, defaultMode, { defaultMode, otherModes }, 1 + VAR_SCHEMES_COUNT(otherModes) },
#include "BRCryptoConfig.h"
    };
    size_t NUMBER_OF_MODES = sizeof (modeSpecs) / sizeof (struct SyncModeSpecification);

    assert (NULL != networksCount);
    *networksCount = NUMBER_OF_NETWORKS;
    BRCryptoNetwork *networks = calloc (NUMBER_OF_NETWORKS, sizeof (BRCryptoNetwork));

    for (size_t networkIndex = 0; networkIndex < NUMBER_OF_NETWORKS; networkIndex++) {
        struct NetworkSpecification *networkSpec = &networkSpecifications[networkIndex];

        BRCryptoNetwork network = cryptoNetworkCreateBuiltin (networkSpec->symbol,
                                                              networkSpec->networkId,
                                                              networkSpec->name);;

        BRCryptoCurrency currency = NULL;

        BRArrayOf(BRCryptoUnit) units;
        array_new (units, 5);

        BRArrayOf(BRCryptoNetworkFee) fees;
        array_new (fees, 3);

        // Create the currency
        for (size_t currencyIndex = 0; currencyIndex < NUMBER_OF_CURRENCIES; currencyIndex++) {
            struct CurrencySpecification *currencySpec = &currencySpecifications[currencyIndex];
            if (0 == strcmp (networkSpec->networkId, currencySpec->networkId)) {
                currency = cryptoCurrencyCreate (currencySpec->currencyId,
                                                 currencySpec->name,
                                                 currencySpec->code,
                                                 currencySpec->type,
                                                 currencySpec->address);

                BRCryptoUnit unitBase    = NULL;
                BRCryptoUnit unitDefault = NULL;

                // Create the units
                for (size_t unitIndex = 0; unitIndex < NUMBER_OF_UNITS; unitIndex++) {
                    struct UnitSpecification *unitSpec = &unitSpecifications[unitIndex];
                    if (0 == strcmp (currencySpec->currencyId, unitSpec->currencyId)) {
                        if (NULL == unitBase) {
                            assert (0 == unitSpec->decimals);
                            unitBase = cryptoUnitCreateAsBase (currency,
                                                               unitSpec->code,
                                                               unitSpec->name,
                                                               unitSpec->symbol);
                            array_add (units, cryptoUnitTake (unitBase));
                        }
                        else {
                            BRCryptoUnit unit = cryptoUnitCreate (currency,
                                                                  unitSpec->code,
                                                                  unitSpec->name,
                                                                  unitSpec->symbol,
                                                                  unitBase,
                                                                  unitSpec->decimals);
                            array_add (units, unit);

                            if (NULL == unitDefault || cryptoUnitGetBaseDecimalOffset(unit) > cryptoUnitGetBaseDecimalOffset(unitDefault)) {
                                if (NULL != unitDefault) cryptoUnitGive(unitDefault);
                                unitDefault = cryptoUnitTake(unit);
                            }
                        }
                    }
                }

                if (0 == strcmp ("native", currencySpec->type))
                    cryptoNetworkSetCurrency (network, currency);

                cryptoNetworkAddCurrency (network, currency, unitBase, unitDefault);

                for (size_t unitIndex = 0; unitIndex < array_count(units); unitIndex++) {
                    cryptoNetworkAddCurrencyUnit (network, currency, units[unitIndex]);
                    cryptoUnitGive (units[unitIndex]);
                }
                array_clear (units);

                cryptoUnitGive(unitBase);
                cryptoUnitGive(unitDefault);
                cryptoCurrencyGive(currency);
            }
        }

        // Create the Network Fees
        BRCryptoUnit feeUnit = cryptoNetworkGetUnitAsBase (network, network->currency);
        for (size_t networkFeeIndex = 0; networkFeeIndex < NUMBER_OF_FEES; networkFeeIndex++) {
            struct NetworkFeeSpecification *networkFeeSpec = &networkFeeSpecifications[networkFeeIndex];
            if (0 == strcmp (networkSpec->networkId, networkFeeSpec->networkId)) {
                BRCryptoAmount pricePerCostFactor = cryptoAmountCreateString (networkFeeSpec->amount,
                                                                              CRYPTO_FALSE,
                                                                              feeUnit);
                BRCryptoNetworkFee fee = cryptoNetworkFeeCreate (networkFeeSpec->confirmationTimeInMilliseconds,
                                                                 pricePerCostFactor,
                                                                 feeUnit);
                array_add (fees, fee);

                cryptoAmountGive(pricePerCostFactor);
            }
        }
        cryptoUnitGive(feeUnit);

        cryptoNetworkSetNetworkFees (network, fees, array_count(fees));
        for (size_t index = 0; index < array_count(fees); index++)
            cryptoNetworkFeeGive (fees[index]);
        array_free(fees);

        // Fill out the Address Schemes
        for (size_t schemeIndex = 0; schemeIndex < NUMBER_OF_SCHEMES; schemeIndex++) {
            struct AddressSchemeSpecification *schemeSpec = &addressSchemeSpecs[schemeIndex];
            if (0 == strcmp (networkSpec->networkId, schemeSpec->networkId)) {
                for (size_t index = 0; index < schemeSpec->numberOfSchemes; index++)
                    cryptoNetworkAddSupportedAddressScheme(network, schemeSpec->schemes[index]);
                network->defaultAddressScheme = schemeSpec->defaultScheme;
            }
        }

        // Fill out the sync modes
        for (size_t modeIndex = 0; modeIndex < NUMBER_OF_MODES; modeIndex++) {
            struct SyncModeSpecification *modeSpec = &modeSpecs[modeIndex];
            if (0 == strcmp (networkSpec->networkId, modeSpec->networkId)) {
                for (size_t index = 0; index < modeSpec->numberOfModes; index++)
                    cryptoNetworkAddSupportedSyncMode (network, modeSpec->modes[index]);
                network->defaultSyncMode = modeSpec->defaultMode;
            }
        }

        array_free (units);

        cryptoNetworkSetConfirmationsUntilFinal (network, networkSpec->confirmations);
        cryptoNetworkSetHeight (network, networkSpec->height);

        networks[networkIndex] = network;

#define SHOW_BUILTIN_CURRENCIES DEBUG
#if defined (SHOW_BUILTIN_CURRENCIES)
        printf ("== Network: %s, '%s'\n", network->uids, network->name);
        for (size_t ai = 0; ai < array_count(network->associations); ai++) {
            BRCryptoCurrencyAssociation a = network->associations[ai];
            printf ("    Currency: %s, '%s'\n", cryptoCurrencyGetUids(a.currency), cryptoCurrencyGetName(a.currency));
            printf ("    Base Unit: %s\n", cryptoUnitGetUids(a.baseUnit));
            printf ("    Default Unit: %s\n", cryptoUnitGetUids(a.defaultUnit));
            printf ("    Units:\n");
            for (size_t ui = 0; ui < array_count(a.units); ui++) {
                BRCryptoUnit u = a.units[ui];
                printf ("      %s, '%s', %5s\n", cryptoUnitGetUids (u), cryptoUnitGetName(u), cryptoUnitGetSymbol(u));
            }
            printf ("\n");
        }
#endif
    }

    return networks;
}

extern BRCryptoNetwork
cryptoNetworkFindBuiltin (const char *uids) {
    size_t networksCount = 0;
    BRCryptoNetwork *networks = cryptoNetworkInstallBuiltins (&networksCount);
    BRCryptoNetwork network = NULL;

    for (size_t index = 0; index < networksCount; index++) {
        if (NULL == network && 0 == strcmp (uids, networks[index]->uids))
            network = cryptoNetworkTake (networks[index]);
        cryptoNetworkGive(networks[index]);
    }
    free (networks);

    return network;
}

