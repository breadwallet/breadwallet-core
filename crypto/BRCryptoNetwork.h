//
//  BRCryptoNetwork.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRCryptoNetwork_h
#define BRCryptoNetwork_h

#include "BRCryptoCurrency.h"
#include "BRCryptoUnit.h"

#include "../support/BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#if 0
    typedef void  *BRCryptoNetworkListenerContext;
    typedef void (*BRCryptoNetworkListenerCreated) (BRCryptoNetworkListenerContext context,
                                                    BRCryptoNetwork network);
    typedef void (*BRCryptoNetworkListenerCurrency) (BRCryptoNetworkListenerContext context,
                                                     BRCryptoNetwork network,
                                                     BRCryptoCurrency currency);

    typedef struct {
        BRCryptoNetworkListenerContext context;
        BRCryptoNetworkListenerCreated created;
 //       BRCryptoNetworkListenerCurrency currencyAdded;
//        BRCryptoNetworkListenerCurrency currencyDeleted;
    } BRCryptoNetworkListener;
#endif
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

    extern BRCryptoBlockChainHeight
    cryptoNetworkGetHeight (BRCryptoNetwork network);


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

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoNetwork, cryptoNetwork);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoNetwork_h */
