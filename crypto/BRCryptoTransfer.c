//
//  BRCryptoTransfer.c
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

#include "BRCryptoTransfer.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "bitcoin/BRTransaction.h"
#include "support/BRAddress.h"
#include "ethereum/BREthereum.h"
#include "ethereum/ewm/BREthereumTransfer.h"

/**
 *
 */
typedef struct {
    uint64_t blockNumber;
    uint64_t transactionIndex;
    uint64_t timestamp;
    BRCryptoAmount fee; // ouch; => cant be a struct
} BRCryptoTransferConfirmation;

struct BRCryptoTransferRecord {
    BRCryptoBlockChainType type;
    union {
        BRTransaction *btc;
        BREthereumTransfer eth;
    } u;

    BRCryptoWallet wallet;
    BRCryptoCurrency currency;
};

static BRCryptoTransfer
cryptoTransferCreateInternal (BRCryptoBlockChainType type,
                              BRCryptoWallet wallet) {
    BRCryptoTransfer transfer = malloc (sizeof (struct BRCryptoTransferRecord));

    transfer->type = type;
    transfer->wallet = wallet;  // TODO: RefCount - Circularity....
    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreate (BRCryptoWallet wallet) {
    BRCryptoBlockChainType type = cryptoWalletGetType (wallet);
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (type, wallet);

    switch (type) {

        case BLOCK_CHAIN_TYPE_BTC:
            transfer->u.btc = NULL;
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            transfer->u.eth = NULL;
//            transfer->u.eth = ewmWalletCreateTransfer (...)

            break;
        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
    // BTC, ETH
    return transfer;
}

extern BRCryptoWallet
cryptoTransferGetWallet (BRCryptoTransfer transfer) {
    return transfer->wallet;
}

extern BRCryptoAddress
cryptoTransferGetSourceAddress (BRCryptoTransfer transfer) {
    switch (transfer->type) {

        case BLOCK_CHAIN_TYPE_BTC:
            return NULL;
        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAddressCreateAsETH (transferGetSourceAddress (transfer->u.eth));
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAddress
cryptoTransferGetTargetAddress (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return NULL;
        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAddressCreateAsETH (transferGetTargetAddress (transfer->u.eth));
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAmount
cryptoTransferGetAmount (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return NULL;
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumAmount amount = transferGetAmount(transfer->u.eth);
            switch (amountGetType(amount)) {
                case AMOUNT_ETHER:
                    return cryptoAmountCreate (transfer->currency,
                                               CRYPTO_FALSE,
                                               etherGetValue(amountGetEther(amount), WEI));
                case AMOUNT_TOKEN:
                    return NULL;

                default:
                    return NULL;
            }
        }
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAmount
cryptoTransferGetFee (BRCryptoTransfer transfer) {
    BRCryptoWallet wallet = transfer->wallet;
    BRCryptoUnit   unit   = cryptoWalletGetUnitForFee (wallet);

    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return cryptoAmountCreateInteger (0, unit);

        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAmountCreateInteger (0, unit);

        case BLOCK_CHAIN_TYPE_GEN:
            return cryptoAmountCreateInteger (0, unit);
    }
}

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


struct BRCryptoTransferHashRecord {
    BRCryptoBlockChainType type;
    union {
        UInt256 btc;
        BREthereumHash eth;
    } u;
};

