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

extern char *
cryptoTransferStateGetErrorMessage (BRCryptoTransferState state) {
    return strdup (state.u.errorred.message);
}

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

    BRCryptoTransferState state;
    BRCryptoCurrency currency;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoTransfer, cryptoTransfer)

static BRCryptoTransfer
cryptoTransferCreateInternal (BRCryptoBlockChainType type,
                              BRCryptoCurrency currency) {
    BRCryptoTransfer transfer = malloc (sizeof (struct BRCryptoTransferRecord));

    transfer->state = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_CREATED };
    transfer->type  = type;
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
    if (BLOCK_CHAIN_TYPE_BTC == transfer->type) BRTransactionFree (transfer->u.btc.tid);
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

static BRCryptoAmount
cryptoTransferGetAmountAsSign (BRCryptoTransfer transfer, BRCryptoBoolean isNegative) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t fee = BRWalletFeeForTx (wid, tid);
            if (UINT64_MAX == fee) fee = 0;

            uint64_t recv = BRWalletAmountReceivedFromTx (wid, tid);
            uint64_t send = BRWalletAmountSentByTx (wid, tid);

            switch (cryptoTransferGetDirection(transfer)) {
                case CRYPTO_TRANSFER_RECOVERED:
                    return cryptoAmountCreate (transfer->currency,
                                               isNegative,
                                               createUInt256(send));
                case CRYPTO_TRANSFER_SENT:
                    return cryptoAmountCreate (transfer->currency,
                                               isNegative,
                                               createUInt256(send - fee - recv));
                case CRYPTO_TRANSFER_RECEIVED:
                    return cryptoAmountCreate (transfer->currency,
                                               isNegative,
                                               createUInt256(recv));
                default: assert(0);
            }
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumAmount amount = transferGetAmount(transfer->u.eth.tid);
            switch (amountGetType(amount)) {
                case AMOUNT_ETHER:
                    return cryptoAmountCreate (transfer->currency,
                                               isNegative,
                                               etherGetValue(amountGetEther(amount), WEI));
                case AMOUNT_TOKEN:
                    return cryptoAmountCreate (transfer->currency,
                                               isNegative,
                                               amountGetTokenQuantity(amount).valueAsInteger);

                default: assert(0);
            }
        }
        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAmount
cryptoTransferGetAmount (BRCryptoTransfer transfer) {
    return cryptoTransferGetAmountAsSign (transfer,
                                          CRYPTO_FALSE);
}

extern BRCryptoAmount
cryptoTransferGetAmountDirected (BRCryptoTransfer transfer) {
    switch (cryptoTransferGetDirection(transfer)) {
        case CRYPTO_TRANSFER_RECOVERED: {
            return cryptoAmountCreate (transfer->currency,
                                       CRYPTO_FALSE,
                                       UINT256_ZERO);
        }
        case CRYPTO_TRANSFER_SENT: {
            return cryptoTransferGetAmountAsSign (transfer,
                                                  CRYPTO_TRUE);
        }
        case CRYPTO_TRANSFER_RECEIVED: {
            return cryptoTransferGetAmountAsSign (transfer,
                                                  CRYPTO_FALSE);
        }
        default: assert(0);
    }
}

extern BRCryptoAmount
cryptoTransferGetFee (BRCryptoTransfer transfer) { // Pass in 'currency' as blockchain baseUnit
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t fee = BRWalletFeeForTx (wid, tid);
            if (UINT64_MAX == fee) fee = 0;

            switch (cryptoTransferGetDirection(transfer)) {
                case CRYPTO_TRANSFER_RECOVERED:
                    return cryptoAmountCreate (transfer->currency,
                                               CRYPTO_FALSE,
                                               createUInt256(fee));
                case CRYPTO_TRANSFER_SENT:
                    return cryptoAmountCreate (transfer->currency,
                                               CRYPTO_FALSE,
                                               createUInt256(fee));
                case CRYPTO_TRANSFER_RECEIVED:
                    return cryptoAmountCreate (transfer->currency,
                                               CRYPTO_FALSE,
                                               UINT256_ZERO);
                default: assert(0);
            }
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

//extern BRCryptoBoolean
//cryptoTransferExtractConfirmation (BRCryptoTransfer transfer,
//                                   uint64_t *blockNumber,
//                                   uint64_t *transactionIndex,
//                                   uint64_t *timestamp,
//                                   BRCryptoAmount *fee) {
//    if (CRYPTO_TRANSFER_STATE_INCLUDED != transfer->state) return CRYPTO_FALSE;
//
//    if (NULL != blockNumber) *blockNumber = 0;
//    if (NULL != transactionIndex) *transactionIndex = 0;
//    if (NULL != timestamp) *timestamp = 0;
//    if (NULL != fee) *fee = cryptoTransferGetFee (transfer);
//
//    return CRYPTO_TRUE;
//}

extern BRCryptoTransferState
cryptoTransferGetState (BRCryptoTransfer transfer) {
    return transfer->state;
}

private_extern void
cryptoTransferSetState (BRCryptoTransfer transfer,
                        BRCryptoTransferState state) {
    transfer->state = state;
}

extern BRCryptoTransferDirection
cryptoTransferGetDirection (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t send = BRWalletAmountSentByTx (wid, tid);
            if (0 == send) {
                return CRYPTO_TRANSFER_RECEIVED;
            }

            uint64_t fee = BRWalletFeeForTx (wid, tid);
            if (UINT64_MAX == fee) {
                fee = 0;
            }

            uint64_t recv = BRWalletAmountReceivedFromTx (wid, tid);
            if ((send - fee) == recv) {
                return CRYPTO_TRANSFER_RECOVERED;
            } else if ((send - fee) > recv) {
                return CRYPTO_TRANSFER_SENT;
            }

            return CRYPTO_TRANSFER_RECEIVED;
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumAddress source = ewmTransferGetSource (ewm, tid);
            BREthereumAddress target = ewmTransferGetTarget (ewm, tid);

            BREthereumAccount account = ewmGetAccount (ewm);
            BREthereumAddress address = accountGetPrimaryAddress (account);

            BREthereumBoolean accountIsSource = addressEqual (source, address);
            BREthereumBoolean accountIsTarget = addressEqual (target, address);

            if (accountIsSource == ETHEREUM_BOOLEAN_TRUE && accountIsTarget == ETHEREUM_BOOLEAN_TRUE) {
                return CRYPTO_TRANSFER_RECOVERED;
            } else if (accountIsSource == ETHEREUM_BOOLEAN_TRUE && accountIsTarget == ETHEREUM_BOOLEAN_FALSE) {
                return CRYPTO_TRANSFER_SENT;
            } else if (accountIsSource == ETHEREUM_BOOLEAN_FALSE && accountIsTarget == ETHEREUM_BOOLEAN_TRUE) {
                return CRYPTO_TRANSFER_RECEIVED;
            }

            assert(0);
        }

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
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = transfer->u.btc.wid;
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t fee = BRWalletFeeForTx (wid, tid);
            uint64_t feePerKb = DEFAULT_FEE_PER_KB;
            if (UINT64_MAX != fee) {
                // round to nearest satoshi per kb
                uint64_t size = BRTransactionVSize (tid);
                feePerKb = ((fee * 1000) + (size / 2)) / size;
            }

            return cryptoFeeBasisCreateAsBTC (feePerKb);
        }
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

private_extern BRTransaction *
cryptoTransferAsBTC (BRCryptoTransfer transfer) {
    assert (BLOCK_CHAIN_TYPE_BTC ==  transfer->type);
    return transfer->u.btc.tid;
}

private_extern BREthereumTransfer
cryptoTransferAsETH (BRCryptoTransfer transfer) {
    assert (BLOCK_CHAIN_TYPE_ETH ==  transfer->type);
    return transfer->u.eth.tid;
}

private_extern BRCryptoBoolean
cryptoTransferHasBTC (BRCryptoTransfer transfer,
                      BRTransaction *btc) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == transfer->type && (BRTransactionIsSigned (transfer->u.btc.tid)
                                                                         ? BRTransactionEq (btc, transfer->u.btc.tid)
                                                                         : btc == transfer->u.btc.tid));
}

private_extern BRCryptoBoolean
cryptoTransferHasETH (BRCryptoTransfer transfer,
                      BREthereumTransfer eth) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == transfer->type && eth == transfer->u.eth.tid);
}

static int
cryptoTransferEqualAsBTC (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return (t1->u.btc.wid == t2->u.btc.wid
            && (BRTransactionIsSigned (t1->u.btc.tid)
                ? BRTransactionEq (t1->u.btc.tid, t2->u.btc.tid)
                : t1->u.btc.tid == t2->u.btc.tid));
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

