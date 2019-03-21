//
//  BRCryptoTransfer.h
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

#ifndef BRCryptoTransfer_h
#define BRCryptoTransfer_h

#include "BRCryptoAddress.h"
#include "BRCryptoAmount.h"

typedef struct BRCryptoWalletRecord *BRCryptoWallet;

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CRYPTO_TRANSFER_CREATED,
        CRYPTO_TRANSFER_SIGNED,
        CRYPTO_TRANSFER_SUBMITTED,
        CRYPTO_TRANSFER_INCLUDED,
        CRYPTO_TRANSFER_ERRORRED,
        CRYPTO_TRANSFER_DELETED
    } BRCryptoTransferState;

     typedef struct BRCryptoTransferRecord *BRCryptoTransfer;

    extern BRCryptoWallet
    cryptoTransferGetWallet (BRCryptoTransfer transfer);

    extern BRCryptoAddress
    cryptoTransferGetSourceAddress (BRCryptoTransfer transfer);

    extern BRCryptoAddress
    cryptoTransferGetTargetAddress (BRCryptoTransfer transfer);

    extern BRCryptoAmount
    cryptoTransferGetAmount (BRCryptoTransfer transfer);

    extern BRCryptoAmount
    cryptoTransferGetFee (BRCryptoTransfer transfer);

    extern BRCryptoBoolean
    cryptoTransferExtractConfirmation (BRCryptoTransfer transfer,
                                       uint64_t *blockNumber,
                                       uint64_t *transactionIndex,
                                       uint64_t *timestamp,
                                       BRCryptoAmount *fee);

    extern BRCryptoBoolean
    cryptoTransferIsSent (BRCryptoTransfer transfer);

    extern BRCryptoTransferState
    cryptoTransferGetState (BRCryptoTransfer transfer);

    // hash
    typedef struct BRCryptoTransferHashRecord *BRCryptoTransferHash;


#ifdef __cplusplus
}
#endif

#endif /* BRCryptoTransfer_h */
