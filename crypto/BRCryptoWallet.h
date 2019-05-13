//
//  BRCryptoWallet.h
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

#ifndef BRCryptoWallet_h
#define BRCryptoWallet_h

#include "BRCryptoFeeBasis.h"
#include "BRCryptoTransfer.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoWalletRecord *BRCryptoWallet;

    extern BRCryptoCurrency
    cryptoWalletGetCurrency (BRCryptoWallet wallet);

    extern BRCryptoUnit
    cryptoWalletGetUnitForFee (BRCryptoWallet wallet);
    
    extern BRCryptoAmount
    cryptoWalletGetBalance (BRCryptoWallet wallet);

    extern size_t
    cryptoWalletGetTransferCount (BRCryptoWallet wallet);

    extern void
    cryptoWalletAddTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer);
    
    extern BRCryptoTransfer
    cryptoWalletGetTransfer (BRCryptoWallet wallet, size_t index);

    extern BRCryptoAddress
    cryptoWalletGetAddress (BRCryptoWallet wallet);

    extern BRCryptoFeeBasis
    cryptoWalletGetDefaultFeeBasis (BRCryptoWallet wallet);

    extern void
    cryptoWalletSetDefaultFeeBasis (BRCryptoWallet wallet,
                                    BRCryptoFeeBasis feeBasis);

    DECLARE_CRYPTO_GIVE_TAKE (BRCryptoWallet, cryptoWallet);

    extern BRCryptoTransfer
    cryptoWalletCreateTransfer (BRCryptoWallet wallet,
                                BRCryptoAddress target,
                                BRCryptoAmount amount,
                                BRCryptoFeeBasis feeBasis);
    extern BRCryptoAmount
    cryptoWalletEstimateFee (BRCryptoWallet wallet,
                             BRCryptoAmount amount,
                             BRCryptoFeeBasis feeBasis,
                             BRCryptoUnit feeUnit); // for fee

    extern BRCryptoBoolean
    cryptoWalletEqual (BRCryptoWallet w1, BRCryptoWallet w2);

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoWallet_h */
