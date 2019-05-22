//
//  BRCryptoSystem.h
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

#ifndef BRCryptoSystem_h
#define BRCryptoSystem_h

#include "BRCryptoNetwork.h"
#include "BRCryptoCurrency.h"
#include "BRCryptoWalletManager.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoSystemRecord *BRCryptoSystem;

    extern BRCryptoSystem
    cryptoSystemCreate (BRCryptoAccount account,
                        const char *path);

    extern const char *
    cryptoSystemGetPersistencePath (BRCryptoSystem system);
    
    extern void
    cryptoSystemStart (BRCryptoSystem system);

    extern void
    cryptoSystemStop (BRCryptoSystem system);

    extern BRCryptoWalletManager
    cryptoSystemCreateWalletManager (BRCryptoSystem system,
                                     BRCryptoCurrency currency,
                                     BRCryptoNetwork network,
                                     BRSyncMode mode);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoSystem_h */
