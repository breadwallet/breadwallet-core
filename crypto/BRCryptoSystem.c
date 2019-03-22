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

static void
cryptoSystemRelease (BRCryptoSystem system) {
    for (size_t index = 0; index < array_count(system->networks); index++)
        cryptoNetworkGive (system->networks[index]);
    array_free (system->networks);

    free (system->path);
    free (system);
}

extern const char *
cryptoSystemGetPersistencePath (BRCryptoSystem system) {
    return system->path;
}

extern void
cryptoSystemStart (BRCryptoSystem system) {

}

extern void
cryptoSystemStop (BRCryptoSystem system) {
}

extern BRCryptoWalletManager
cryptoSystemCreateWalletManager (BRCryptoSystem system,
                                 BRCryptoNetwork network,
                                 BRCryptoCurrency currency,
                                 BRCryptoSyncMode mode) {

    return NULL;
}
