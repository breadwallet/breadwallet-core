//
//  BRCryptoWallet.c
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

#include "BRCryptoWallet.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "bitcoin/BRWallet.h"
#include "ethereum/BREthereum.h"

/**
 *
 */
struct BRCryptoWalletRecord {
    BRCryptoBlockChainType type;
    union {
        BRWallet *btc;
        BREthereumWallet eth;
    } u;

    BRCryptoUnit unit;  // baseUnit
    BRArrayOf (BRCryptoTransfer) transfers;
};

extern BRCryptoCurrency
cryptoWalletGetCurrency (BRCryptoWallet wallet) {
    return cryptoUnitGetCurrency(wallet->unit);
}

extern BRCryptoAmount
cryptoWalletGetBalance (BRCryptoWallet wallet) {
    UInt256 value = UINT256_ZERO;
    return cryptoAmountCreate (cryptoUnitGetCurrency (wallet->unit), CRYPTO_FALSE, value);
}

extern BRArrayOf(BRCryptoTransfer)
cryptoWalletGetTransfers (BRCryptoWallet wallet) {
    return wallet->transfers;
}

extern BRCryptoAddress
cryptoWalletGetAddress (BRCryptoWallet wallet) {
    return cryptoAddressCreate("no address");
}
