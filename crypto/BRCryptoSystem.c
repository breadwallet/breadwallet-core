//
//  BRCryptoSystem.c
//  BRCore
//
//  Created by Ed Gamble on 3/22/19.
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

#include "BRCryptoSystem.h"
#include "BRCryptoPrivate.h"

#define CRYPTO_SYSTEM_DEFAULT_NETWORKS              (5)
struct BRCryptoSystemRecord {
    BRCryptoAccount account;
    char *path;
    BRArrayOf(BRCryptoNetwork) networks;

    // WalletManagers
};

extern BRCryptoSystem
cryptoSystemCreate (BRCryptoAccount account,
                    const char *path) {
    BRCryptoSystem system = malloc (sizeof (struct BRCryptoSystemRecord));

    system->account = account;
    system->path = strdup (path);
    array_new (system->networks, CRYPTO_SYSTEM_DEFAULT_NETWORKS);

    return system;
}

#if 0
static void
cryptoSystemRelease (BRCryptoSystem system) {
    for (size_t index = 0; index < array_count(system->networks); index++)
        cryptoNetworkGive (system->networks[index]);
    array_free (system->networks);

    free (system->path);
    free (system);
}
#endif

extern const char *
cryptoSystemGetPersistencePath (BRCryptoSystem system) {
    return system->path;
}

extern void
cryptoSystemStart (BRCryptoSystem system) {

    // Query the BlockchainDB - determine the networks.

    BRCryptoNetwork networks[] = {
        // Bitcoin
        cryptoNetworkCreateAsBTC("BTC Mainnet", 0x00, BRMainNetParams),
        cryptoNetworkCreateAsBTC("BTC Testnet", 0x40, BRTestNetParams),

        // Bitcash
        cryptoNetworkCreateAsBTC("BCH Mainnet", 0x00, BRBCashParams),
        cryptoNetworkCreateAsBTC("BCH Testnet", 0x40, BRBCashTestNetParams),

        // Ethereum
        cryptoNetworkCreateAsETH ("ETH Mainnet", 1, ethereumMainnet),
        cryptoNetworkCreateAsETH ("ETH Ropsten", 3, ethereumTestnet),
        cryptoNetworkCreateAsETH ("ETH Rinkeby", 4, ethereumRinkeby)
    };

    BRArrayOf(BRCryptoUnit) units;
    array_new (units, 3);

    BRCryptoCurrency BTC = cryptoCurrencyCreate ("Bitcoin", "BTC", "native");
    BRCryptoUnit BTC_SAT = cryptoUnitCreateAsBase (BTC, "SAT", "sat");
    BRCryptoUnit BTC_BTC = cryptoUnitCreate (BTC, "BTC", "₿", BTC_SAT, 8);
    BRArrayOf(BRCryptoUnit) BTC_UNITS;
    array_new (BTC_UNITS, 2);
    array_add (BTC_UNITS, BTC_SAT);
    array_add (BTC_UNITS, BTC_BTC);

    BRCryptoCurrency BCH = cryptoCurrencyCreate ("Bitcash", "BCH", "native");
    BRCryptoUnit BCH_SAT = cryptoUnitCreateAsBase (BCH, "SAT", "sat");
    BRCryptoUnit BCH_BTC = cryptoUnitCreate (BCH, "BTH", "₿", BCH_SAT, 8);
    BRArrayOf(BRCryptoUnit) BCH_UNITS;
    array_new (BCH_UNITS, 2);
    array_add (BCH_UNITS, BCH_SAT);
    array_add (BCH_UNITS, BCH_BTC);

    BRCryptoCurrency ETH   = cryptoCurrencyCreate ("Ethereum", "ETH", "native");
    BRCryptoUnit ETH_WEI   = cryptoUnitCreateAsBase (ETH, "WEI", "wei");
    BRCryptoUnit ETH_GWEI  = cryptoUnitCreate (ETH, "GWEI",  "WEI", ETH_WEI,  9);
    BRCryptoUnit ETH_ETHER = cryptoUnitCreate (ETH, "ETHER", "Ξ",   ETH_WEI, 18);
    BRArrayOf(BRCryptoUnit) ETH_UNITS;
    array_new (ETH_UNITS, 3);
    array_add (ETH_UNITS, ETH_WEI);
    array_add (ETH_UNITS, ETH_GWEI);
    array_add (ETH_UNITS, ETH_ETHER);

    BRCryptoCurrency BRD = cryptoCurrencyCreate("BRD", "BRD", "erc20");
    BRCryptoUnit BRD_INTEGER = cryptoUnitCreateAsBase (BRD, "BRD_INTEGER", "BRD_INT");
    BRCryptoUnit BRD_BRD     = cryptoUnitCreate (BRD, "BRD", "BRD", BRD_INTEGER, 18);
    BRArrayOf(BRCryptoUnit) BRD_UNITS;
    array_new (BRD_UNITS, 2);
    array_add (BRD_UNITS, BRD_INTEGER);
    array_add (BRD_UNITS, BRD_BRD);


    array_new (units, 3);
    array_add_array (units, BTC_UNITS, array_count(BTC_UNITS));
    cryptoNetworkAddCurrency (networks[0], BTC, BTC_SAT, BTC_BTC, units);
    cryptoNetworkSetCurrency (networks[0], BTC);

    array_new (units, 3);
    array_add_array (units, BTC_UNITS, array_count(BTC_UNITS));
    cryptoNetworkAddCurrency (networks[1], BTC, BTC_SAT, BTC_BTC, units);
    cryptoNetworkSetCurrency (networks[1], BTC);

    array_new (units, 3);
    array_add_array (units, BCH_UNITS, array_count(BCH_UNITS));
    cryptoNetworkAddCurrency (networks[2], BCH, BCH_SAT, BCH_BTC, units);
    cryptoNetworkSetCurrency (networks[2], BCH);

    array_new (units, 3);
    array_add_array (units, BCH_UNITS, array_count(BCH_UNITS));
    cryptoNetworkAddCurrency (networks[3], BCH, BCH_SAT, BCH_BTC, units);
    cryptoNetworkSetCurrency (networks[3], BCH);

    array_new (units, 3);
    array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
    cryptoNetworkAddCurrency (networks[4], ETH, ETH_WEI, ETH_ETHER, units);
    array_new (units, 3);
    array_add_array (units, BRD_UNITS, array_count(BRD_UNITS));
    cryptoNetworkAddCurrency (networks[4], BRD, BRD_INTEGER, BRD_BRD, units);
    cryptoNetworkSetCurrency (networks[4], ETH);

    array_new (units, 3);
    array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
    cryptoNetworkAddCurrency (networks[5], ETH, ETH_WEI, ETH_ETHER, units);
    array_new (units, 3);
    array_add_array (units, BRD_UNITS, array_count(BRD_UNITS));
    cryptoNetworkAddCurrency (networks[5], BRD, BRD_INTEGER, BRD_BRD, units);
    cryptoNetworkSetCurrency (networks[5], ETH);

    array_new (units, 3);
    array_add_array (units, ETH_UNITS, array_count(ETH_UNITS));
    cryptoNetworkAddCurrency (networks[6], ETH, ETH_WEI, ETH_ETHER, units);
    // No BRD
    cryptoNetworkSetCurrency (networks[6], ETH);

    array_free (units);
    array_free (BRD_UNITS); array_free (ETH_UNITS); array_free (BCH_UNITS); array_free (BTC_UNITS);

    // Announce Networks
    for (size_t index = 0; index <= 6; index++)
        cryptoNetworkAnnounce(networks[index]);
}

extern void
cryptoSystemStop (BRCryptoSystem system) {
}

extern BRCryptoWalletManager
cryptoSystemCreateWalletManager (BRCryptoSystem system,
                                 BRCryptoCurrency currency,
                                 BRCryptoNetwork network,
                                 BRCryptoSyncMode mode) {

    return NULL;
}
