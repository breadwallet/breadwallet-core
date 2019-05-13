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

#include "support/BRAddress.h"
#include "bitcoin/BRTransaction.h"
#include "ethereum/util/BRUtil.h"
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

static void
cryptoTransferRelease (BRCryptoTransfer transfer);

struct BRCryptoTransferRecord {
    BRCryptoBlockChainType type;
    union {
        struct {
            BRWallet *wid;
            BRTransaction *tid;
        } btc;
        struct {
            BREthereumEWM ewm;
            BREthereumTransfer tid;
        } eth;
    } u;

    BRCryptoCurrency currency;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoTransfer, cryptoTransfer)

static BRCryptoTransfer
cryptoTransferCreateInternal (BRCryptoBlockChainType type,
                              BRCryptoCurrency currency) {
    BRCryptoTransfer transfer = malloc (sizeof (struct BRCryptoTransferRecord));

    transfer->type = type;
    transfer->currency = cryptoCurrencyTake(currency);
    transfer->ref = CRYPTO_REF_ASSIGN (cryptoTransferRelease);

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsBTC (BRCryptoCurrency currency,
                           BRWallet *wid,
                           BRTransaction *tid) {
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_BTC, currency);
    transfer->u.btc.wid = wid;
    transfer->u.btc.tid = tid;

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsETH (BRCryptoCurrency currency,
                           BREthereumEWM ewm,
                           BREthereumTransfer tid) {
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_ETH, currency);
    transfer->u.eth.ewm = ewm;
    transfer->u.eth.tid = tid;

    return transfer;
}

static void
cryptoTransferRelease (BRCryptoTransfer transfer) {
    cryptoCurrencyGive (transfer->currency);
    free (transfer);
}

extern BRCryptoBlockChainType
cryptoTransferGetType (BRCryptoTransfer transfer) {
    return transfer->type;
}

extern BRCryptoAddress
cryptoTransferGetSourceAddress (BRCryptoTransfer transfer) {
    switch (transfer->type) {

        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            int sent = UINT64_MAX != BRWalletFeeForTx (wid, tid);

            size_t     inputsCount = tid->inCount;
            BRTxInput *inputs      = tid->inputs;

            int inputsContain = (sent ? 1 : 0);

            for (size_t index = 0; index < inputsCount; index++)
                if (inputsContain == BRWalletContainsAddress(wid, inputs[index].address)) {
                    BRAddress address;
                    memcpy (address.s, inputs[index].address, sizeof (address.s));
                    return cryptoAddressCreateAsBTC (address);
                }
            
            return NULL;
        }
        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAddressCreateAsETH (transferGetSourceAddress (transfer->u.eth.tid));
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAddress
cryptoTransferGetTargetAddress (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            int sent = UINT64_MAX != BRWalletFeeForTx (wid, tid);

            size_t      outputsCount = tid->outCount;
            BRTxOutput *outputs      = tid->outputs;

            int outputsContain = (!sent ? 1 : 0);

            for (size_t index = 0; index < outputsCount; index++)
                if (outputsContain == BRWalletContainsAddress(wid, outputs[index].address)) {
                    BRAddress address;
                    memcpy (address.s, outputs[index].address, sizeof (address.s));
                    return cryptoAddressCreateAsBTC (address);
                }

            return NULL;
        }
        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAddressCreateAsETH (transferGetTargetAddress (transfer->u.eth.tid));
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAmount
cryptoTransferGetAmount (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t fees = BRWalletFeeForTx (wid, tid);
            if (fees == UINT64_MAX) { fees = 0; }

            uint64_t recv = BRWalletAmountReceivedFromTx (wid, tid);
            uint64_t send = BRWalletAmountSentByTx (wid, tid);   // includes fees

            // The value is always positive; it is the value sent from source to target.
            uint64_t value = (0 == fees
                              ? recv - send
                              : (send - fees) - recv);

            return cryptoAmountCreate (transfer->currency, CRYPTO_FALSE, createUInt256(value));
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumAmount amount = transferGetAmount(transfer->u.eth.tid);
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
cryptoTransferGetFee (BRCryptoTransfer transfer) { // Pass in 'currency' as blockchain baseUnit
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t value = BRWalletFeeForTx (wid, tid);

            return cryptoAmountCreate (transfer->currency, CRYPTO_FALSE, createUInt256(value));
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid =transfer->u.eth.tid;
            int overflow = 0;

            BREthereumEther amount = ewmTransferGetFee (ewm, tid, &overflow);
            assert (0 == overflow);

            return cryptoAmountCreate (transfer->currency, CRYPTO_FALSE, amount.valueInWEI);
        }

        case BLOCK_CHAIN_TYPE_GEN:
            return cryptoAmountCreate (transfer->currency, CRYPTO_FALSE, UINT256_ZERO);
    }
}

extern BRCryptoBoolean
cryptoTransferExtractConfirmation (BRCryptoTransfer transfer,
                                   uint64_t *blockNumber,
                                   uint64_t *transactionIndex,
                                   uint64_t *timestamp,
                                   BRCryptoAmount *fee);

extern BRCryptoTransferState
cryptoTransferGetState (BRCryptoTransfer transfer);

extern BRCryptoTransferDirection
cryptoTransferGetDirection (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return CRYPTO_TRANSFER_SENT;

        case BLOCK_CHAIN_TYPE_ETH:
            return CRYPTO_TRANSFER_SENT;

        case BLOCK_CHAIN_TYPE_GEN:
            return CRYPTO_TRANSFER_SENT;
    }
}

extern BRCryptoHash
cryptoTransferGetHash (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRTransaction *tid = transfer->u.btc.tid;

            UInt256 hash = tid->txHash;
            return (1 == UInt256IsZero(hash)
                    ? NULL
                    : cryptoHashCreateAsBTC (hash));
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumHash hash = ewmTransferGetOriginatingTransactionHash (ewm, tid);
            return (ETHEREUM_BOOLEAN_TRUE == hashEqual(hash, hashCreateEmpty())
                    ? NULL
                    : cryptoHashCreateAsETH (hash));
        }

        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoFeeBasis
cryptoTransferGetFeeBasis (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return cryptoFeeBasisCreateAsBTC (DEFAULT_FEE_PER_KB);

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumGas gas = ewmTransferGetGasLimit (ewm, tid);
            BREthereumGasPrice gasPrice = ewmTransferGetGasPrice (ewm, tid, WEI);

            return cryptoFeeBasisCreateAsETH (gas, gasPrice);
        }

        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

static int
cryptoTransferEqualAsBTC (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return (t1->u.btc.wid == t2->u.btc.wid &&
            t1->u.btc.tid == t2->u.btc.tid);
}

static int
cryptoTransferEqualAsETH (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return (t1->u.eth.ewm == t2->u.eth.ewm &&
            t1->u.eth.tid == t2->u.eth.tid);
}

extern BRCryptoBoolean
cryptoTransferEqual (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return AS_CRYPTO_BOOLEAN (t1 == t2 || (t1->type == t2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == t1->type && cryptoTransferEqualAsBTC (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == t1->type && cryptoTransferEqualAsETH (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == t1->type && 0))));
}

