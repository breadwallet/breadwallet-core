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
    
    extern void
    cryptoNetworkDeclareListener (BRCryptoNetworkListener listener);

    extern BRArrayOf(BRCryptoNetwork) networks;

    extern BRCryptoBlockChainType
    cryptoNetworkGetType (BRCryptoNetwork network);
    
    extern const char *
    cryptoNetworkGetName (BRCryptoNetwork network);

    extern BRCryptoBoolean
    cryptoNetworkIsMainnet (BRCryptoNetwork network);

    extern BRCryptoCurrency
    cryptoNetworkGetCurrency (BRCryptoNetwork network);

    extern BRCryptoUnit
    cryptoNetworkGetUnitAsDefault (BRCryptoNetwork network,
                                   BRCryptoCurrency currency);

#if 0
    extern BRCryptoBlockChainHeight
    cryptoNetworkGetHeight (BRCryptoNetwork network);

    extern size_t
    cryptoNetworkGetCurrencyCount (BRCryptoNetwork network);

    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyAt (BRCryptoNetwork network,
                                size_t index);

    extern BRCryptoCurrency
    cryptoNetworkGetCurrencyForSymbol (BRCryptoNetwork network,
                                       const char *symbol);

    extern BRCryptoUnit
    cryptoNetworkGetUnitAsBase (BRCryptoNetwork network,
                                BRCryptoCurrency currency);

    extern size_t
    cryptoNetworkGetUnitCount (BRCryptoNetwork network,
                               BRCryptoCurrency currency);

    extern BRCryptoUnit
    cryptoNetworkGetUnitAt (BRCryptoNetwork network,
                            BRCryptoCurrency currency,
                            size_t index);
#endif
    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoNetwork, cryptoNetwork);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoNetwork_h */
