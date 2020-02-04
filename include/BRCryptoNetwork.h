//
//  BRCryptoNetwork.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoNetwork_h
#define BRCryptoNetwork_h

#include "BRCryptoAmount.h"
#include "BRCryptoSync.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Crypto Network Type
    ///
    /// Try as we might, there are certain circumstances where the type of the network needs to
    /// be known.  Without this enumeration, one uses hack-arounds like:
    ///    "btc" == network.currency.code
    /// So, provide these and expect them to grow.
    ///
    /// Enumerations here need to be consistent with the networks defined in;
    ///    crypto/BRCryptoConfig.h
    ///
    typedef enum {
        CRYPTO_NETWORK_TYPE_BTC,
        CRYPTO_NETWORK_TYPE_BCH,
        CRYPTO_NETWORK_TYPE_ETH,
        CRYPTO_NETWORK_TYPE_XRP,
        // CRYPTO_NETWORK_TYPE_HBAR,
        // CRYPTO_NETWORK_TYPE_XLM,
    } BRCryptoNetworkCanonicalType;

#  define NUMBER_OF_NETWORK_TYPES    (1 + CRYPTO_NETWORK_TYPE_XRP)

    extern const char *
    cryptoNetworkCanonicalTypeString (BRCryptoNetworkCanonicalType type);

    /// MARK: - (Network) Address Scheme

    typedef enum {
        CRYPTO_ADDRESS_SCHEME_BTC_LEGACY,
        CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,
        CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT,
        CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT
    } BRCryptoAddressScheme;

#define NUMBER_OF_ADDRESS_SCHEMES   (1 + CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT)


    // Same as: BRBlockHeight
    typedef uint64_t BRCryptoBlockChainHeight;

    typedef struct BRCryptoNetworkFeeRecord *BRCryptoNetworkFee;

    extern BRCryptoNetworkFee
    cryptoNetworkFeeCreate (uint64_t confirmationTimeInMilliseconds,
                            BRCryptoAmount pricePerCostFactor,
                            BRCryptoUnit   pricePerCostFactorUnit);


    /**
     * The estimated time to confirm a transfer for this network fee
     *
     * @param networkFee the network fee
     *
     * @return time in milliseconds
     */
    extern uint64_t
    cryptoNetworkFeeGetConfirmationTimeInMilliseconds (BRCryptoNetworkFee networkFee);

    extern BRCryptoAmount
    cryptoNetworkFeeGetPricePerCostFactor (BRCryptoNetworkFee networkFee);

    extern BRCryptoUnit
    cryptoNetworkFeeGetPricePerCostFactorUnit (BRCryptoNetworkFee networkFee);

    extern BRCryptoBoolean
    cryptoNetworkFeeEqual (BRCryptoNetworkFee nf1, BRCryptoNetworkFee nf2);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoNetworkFee, cryptoNetworkFee);

    /**
     * A Crypto Network represents a Blockchain.  The blockchains are determined from the
     * BlockChainDB '/blockchains' query; however, networks for testnets are also defined.
     * Thus, the available networks are {btc,bch,eth,...} X {mainnet,testnet}
     *
     * A Crypto Network has a currency which represents the asset used to pay for network fees.
     * For Bitcoin the currency is 'bitcoin'; for Ethereum the currency is 'Ethereum'.
     *
     * A Crypto Network may support more than one currency.  For Ethereum additional currencies
     * include the ERC20 Smart Contracts of interest - for example, BRD.
     *
     * Every Crypto Network's currency has a defined base Unit, default Unit and an arbitrary
     * set of other units.  For Ethereum there are: WEI, ETHER, [WEI, GWEI, ..., ETHER, ...]
     * respectively.
     */
    typedef struct BRCryptoNetworkRecord *BRCryptoNetwork;

    typedef void *BRCryptoNetworkListener;

    extern BRCryptoNetworkCanonicalType
    cryptoNetworkGetCanonicalType (BRCryptoNetwork network);

    extern const char *
    cryptoNetworkGetUids (BRCryptoNetwork network);

    extern const char *
    cryptoNetworkGetName (BRCryptoNetwork network);

    extern BRCryptoBoolean
    cryptoNetworkIsMainnet (BRCryptoNetwork network);

    /**
     * Returns the network's currency.  This is typically (always?) the currency used to pay
     * for network fees.
     
     @param network The network
     @return the network's currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoNetworkGetCurrency (BRCryptoNetwork network);

    extern void
    cryptoNetworkSetCurrency (BRCryptoNetwork network,
                              BRCryptoCurrency currency);

    extern void
    cryptoNetworkAddCurrency (BRCryptoNetwork network,
                              BRCryptoCurrency currency,
                              BRCryptoUnit baseUnit,
                              BRCryptoUnit defaultUnit);


    extern const char *
    cryptoNetworkGetCurrencyCode (BRCryptoNetwork network);

    /**
     * Returns the currency's default unit or NULL
     *
     * @param network the network
     * @param currency the currency desired for the default unit
     *
     * @return the currency's default unit or NULL w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoNetworkGetUnitAsDefault (BRCryptoNetwork network,
                                   BRCryptoCurrency currency);

    /**
     * Returns the currency's base unit or NULL
     *
     * @param network the network
     * @param currency the currency desired for the base unit
     *
     * @return the currency's base unit or NULL w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoNetworkGetUnitAsBase (BRCryptoNetwork network,
                                BRCryptoCurrency currency);

    extern void
    cryptoNetworkAddCurrencyUnit (BRCryptoNetwork network,
                                  BRCryptoCurrency currency,
                                  BRCryptoUnit unit);


    extern BRCryptoBlockChainHeight
    cryptoNetworkGetHeight (BRCryptoNetwork network);

    extern void
    cryptoNetworkSetHeight (BRCryptoNetwork network,
                            BRCryptoBlockChainHeight height);

    extern uint32_t
    cryptoNetworkGetConfirmationsUntilFinal (BRCryptoNetwork network);

    /**
     * Returns the number of network currencies.  This is the index exclusive limit to be used
     * in `cryptoNetworkGetCurrencyAt()`.
     *
     * @param network the network
     *
     * @return number of network currencies.
     */
    extern size_t
    cryptoNetworkGetCurrencyCount (BRCryptoNetwork network);

    /**
     * Returns the network's currency at `index`.  The index must satisfy [0, count) otherwise
     * an assertion is signaled.
     *
     * @param network the network
     * @param index the desired currency index
     *
     * @return The currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyAt (BRCryptoNetwork network,
                                size_t index);

    /**
     * Return 'TRUE' is `network` has `currency`.
     *
     * @param network the network
     *@param currency the currency
     *
     *@return CRYPTO_TRUE if `network` has `currency`.
     */
    extern BRCryptoBoolean
    cryptoNetworkHasCurrency (BRCryptoNetwork network,
                              BRCryptoCurrency currency);

    /**
     * Returns the network's currency with `symbol` or NULL.
     *
     * @param network the network
     * @param index the desired currency's symbol
     *
     * @return The currency w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyForCode (BRCryptoNetwork network,
                                     const char *code);

    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyForUids (BRCryptoNetwork network,
                                     const char *uids);

    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyForIssuer (BRCryptoNetwork network,
                                       const char *issuer);

    /**
     * Returns the number of units for network's `currency`.  This is the index exclusive limit to
     * be used in `cryptoNetworkGetUnitAt()`.
     *
     * @param network the network
     * @param currency the currency
     *
     * @return the number of units for `currency`
     */
    extern size_t
    cryptoNetworkGetUnitCount (BRCryptoNetwork network,
                               BRCryptoCurrency currency);

    /**
     * Returns the currency's unit at `index`.  The index must satisfy [0, count) otherwise an
     * assertion is signaled.
     *
     * @param network the network
     * @param currency the currency
     * @param index the desired unit's index
     *
     * @return the currency unit w/ an incremented reference count (aka 'taken')
     */
    extern BRCryptoUnit
    cryptoNetworkGetUnitAt (BRCryptoNetwork network,
                            BRCryptoCurrency currency,
                            size_t index);

    extern size_t
    cryptoNetworkGetNetworkFeeCount (BRCryptoNetwork network);

    extern BRCryptoNetworkFee
    cryptoNetworkGetNetworkFeeAt (BRCryptoNetwork network,
                                  size_t index);

    extern BRCryptoNetworkFee *
    cryptoNetworkGetNetworkFees (BRCryptoNetwork network,
                                 size_t *count);

    extern void
    cryptoNetworkSetNetworkFees (BRCryptoNetwork network,
                                 const BRCryptoNetworkFee *fees,
                                 size_t count);

    extern void
    cryptoNetworkAddNetworkFee (BRCryptoNetwork network,
                                BRCryptoNetworkFee fee);

    // MARK: - Address Scheme

    extern BRCryptoAddressScheme
    cryptoNetworkGetDefaultAddressScheme (BRCryptoNetwork network);

    extern const BRCryptoAddressScheme *
    cryptoNetworkGetSupportedAddressSchemes (BRCryptoNetwork network,
                                             BRCryptoCount *count);

    extern BRCryptoBoolean
    cryptoNetworkSupportsAddressScheme (BRCryptoNetwork network,
                                        BRCryptoAddressScheme scheme);

    // MARK: - Sync Mode

    extern BRCryptoSyncMode
    cryptoNetworkGetDefaultSyncMode (BRCryptoNetwork network);

    extern const BRCryptoSyncMode *
    cryptoNetworkGetSupportedSyncModes (BRCryptoNetwork network,
                                        BRCryptoCount *count);

    extern BRCryptoBoolean
    cryptoNetworkSupportsSyncMode (BRCryptoNetwork network,
                                   BRCryptoSyncMode scheme);

    extern BRCryptoBoolean
    cryptoNetworkRequiresMigration (BRCryptoNetwork network);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoNetwork, cryptoNetwork);

    extern BRCryptoNetwork *
    cryptoNetworkInstallBuiltins (size_t *networksCount);

    extern BRCryptoNetwork
    cryptoNetworkFindBuiltin (const char *uids);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoNetwork_h */
