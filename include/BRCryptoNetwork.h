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

#include "BRCryptoAddress.h"
#include "BRCryptoAmount.h"

#ifdef __cplusplus
extern "C" {
#endif

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

    extern BRCryptoBlockChainType
    cryptoNetworkGetType (BRCryptoNetwork network);

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

    extern void
    cryptoNetworkSetConfirmationsUntilFinal (BRCryptoNetwork network,
                                             uint32_t confirmationsUntilFinal);

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

    extern BRCryptoAddress
    cryptoNetworkCreateAddressFromString (BRCryptoNetwork network,
                                          const char *string);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoNetwork, cryptoNetwork);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoNetwork_h */
