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
#include "bitcoin/BRWallet.h"
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
            BRTransaction *tid;
            uint64_t fee;
            uint64_t send;
            uint64_t recv;
        } btc;
        struct {
            BREthereumTransfer tid;
            BREthereumAddress accountAddress;
        } eth;
        struct {
            BRGenericWalletManager gwm;
            BRGenericTransfer tid;
        } gen;
    } u;

    BRCryptoAddress sourceAddress;
    BRCryptoAddress targetAddress;
    BRCryptoTransferState state;

    /// The amount's unit.
    BRCryptoUnit unit;

    /// The fee's unit
    BRCryptoUnit unitForFee;

    /// The feeBasis.  We must include this here for at least the case of BTC where the fees
    /// encoded into the BTC-wire-transaction are based on the BRWalletFeePerKB value at the time
    /// that the transaction is created.  Sometime later, when the feeBasis is needed we can't
    /// go to the BTC wallet and expect the FeePerKB to be unchanged.

    /// Actually this can be derived from { btc.fee / txSize(btc.tid), txSize(btc.tid) }
    BRCryptoFeeBasis feeBasisEstimated;
    BRCryptoFeeBasis feeBasisConfirmed;

    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoTransfer, cryptoTransfer)

static BRCryptoTransfer
cryptoTransferCreateInternal (BRCryptoBlockChainType type,
                              BRCryptoUnit unit,
                              BRCryptoUnit unitForFee) {
    BRCryptoTransfer transfer = calloc (1, sizeof (struct BRCryptoTransferRecord));

    transfer->state = (BRCryptoTransferState) { CRYPTO_TRANSFER_STATE_CREATED };
    transfer->type  = type;
    transfer->unit       = cryptoUnitTake(unit);
    transfer->unitForFee = cryptoUnitTake(unitForFee);
    transfer->feeBasisEstimated = NULL;
    transfer->feeBasisConfirmed = NULL;

    transfer->ref = CRYPTO_REF_ASSIGN (cryptoTransferRelease);

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsBTC (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BRWallet *wid,
                           BRTransaction *tid) {
    BRAddressParams  addressParams = BRWalletGetAddressParams (wid);
    BRCryptoTransfer transfer      = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_BTC, unit, unitForFee);
    transfer->u.btc.tid = tid;

    // cache the values that require the wallet
    transfer->u.btc.fee  = BRWalletFeeForTx (wid, tid);
    transfer->u.btc.recv = BRWalletAmountReceivedFromTx (wid, tid);
    transfer->u.btc.send = BRWalletAmountSentByTx (wid, tid);

    {
        size_t     inputsCount = tid->inCount;
        BRTxInput *inputs      = tid->inputs;

        int inputsContain = (UINT64_MAX != transfer->u.btc.fee ? 1 : 0);

        for (size_t index = 0; index < inputsCount; index++) {
            size_t addressSize = BRTxInputAddress (&inputs[index], NULL, 0, addressParams);
            char address [addressSize];

            BRTxInputAddress (&inputs[index], address, addressSize, addressParams);

            if (inputsContain == BRWalletContainsAddress(wid, address)) {
                assert (addressSize < sizeof (BRAddress));
                transfer->sourceAddress = cryptoAddressCreateAsBTC (BRAddressFill (addressParams, address));
                break;
            }
        }
    }

    {
        size_t      outputsCount = tid->outCount;
        BRTxOutput *outputs      = tid->outputs;

        int outputsContain = (UINT64_MAX == transfer->u.btc.fee ? 1 : 0);

        for (size_t index = 0; index < outputsCount; index++) {
            size_t addressSize = BRTxOutputAddress (&outputs[index], NULL, 0, addressParams);
            char address [addressSize];

            BRTxOutputAddress (&outputs[index], address, addressSize, addressParams);

            if (outputsContain == BRWalletContainsAddress(wid, address)) {
                assert (addressSize < sizeof (BRAddress));
                transfer->targetAddress = cryptoAddressCreateAsBTC (BRAddressFill (addressParams, address));
                break;
            }
        }
    }

    //
    // Currently this function, cryptoTransferCreateAsBTC(), is only called in various CWM
    // event handlers based on BTC events.  Thus for a newly created BTC transfer, the
    // BRCryptoFeeBasis is long gone.  The best we can do is reconstruct the feeBasis from the
    // BRTransaction itself.
    //
    uint64_t fee = transfer->u.btc.fee;
    uint32_t feePerKB = 0;  // assume not our transaction (fee == UINT64_MAX)
    uint32_t sizeInByte = (uint32_t) BRTransactionVSize (tid);

    if (UINT64_MAX != fee) {
        // round to nearest satoshi per kb
        feePerKB = (uint32_t) (((1000 * fee) + (sizeInByte/2)) / sizeInByte);
    }

    transfer->feeBasisEstimated = cryptoFeeBasisCreateAsBTC (transfer->unitForFee, feePerKB, sizeInByte);;

    return transfer;
}

private_extern BRCryptoTransfer
cryptoTransferCreateAsETH (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BREthereumEWM ewm,
                           BREthereumTransfer tid,
                           BRCryptoFeeBasis feeBasisEstimated) {
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_ETH, unit, unitForFee);
    transfer->u.eth.tid = tid;

    transfer->sourceAddress = cryptoAddressCreateAsETH (transferGetSourceAddress (tid));
    transfer->targetAddress = cryptoAddressCreateAsETH (transferGetTargetAddress (tid));

    // cache the values that require the ewm
    BREthereumAccount account = ewmGetAccount (ewm);
    transfer->u.eth.accountAddress = accountGetPrimaryAddress (account);

    transfer->feeBasisEstimated = (NULL == feeBasisEstimated ? NULL : cryptoFeeBasisTake(feeBasisEstimated));

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsGEN (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BRGenericWalletManager gwm,
                           BRGenericTransfer tid) {
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_GEN, unit, unitForFee);
    transfer->u.gen.gwm = gwm;
    transfer->u.gen.tid = tid;

    BRGenericFeeBasis gwmFeeBasis = gwmTransferGetFeeBasis (gwm, tid); // Will give ownership
    transfer->feeBasisEstimated = cryptoFeeBasisCreateAsGEN (transfer->unitForFee, gwm, gwmFeeBasis);

    return transfer;
}

static void
cryptoTransferRelease (BRCryptoTransfer transfer) {
    printf ("Transfer: Release\n");
    if (BLOCK_CHAIN_TYPE_BTC == transfer->type) BRTransactionFree (transfer->u.btc.tid);
    if (NULL != transfer->sourceAddress) cryptoAddressGive (transfer->sourceAddress);
    if (NULL != transfer->targetAddress) cryptoAddressGive (transfer->targetAddress);
    cryptoUnitGive (transfer->unit);
    cryptoUnitGive (transfer->unitForFee);
    if (NULL != transfer->feeBasisEstimated) cryptoFeeBasisGive (transfer->feeBasisEstimated);
    if (NULL != transfer->feeBasisConfirmed) cryptoFeeBasisGive (transfer->feeBasisConfirmed);
    memset (transfer, 0, sizeof(*transfer));
    free (transfer);
}

extern BRCryptoBlockChainType
cryptoTransferGetType (BRCryptoTransfer transfer) {
    return transfer->type;
}

extern BRCryptoAddress
cryptoTransferGetSourceAddress (BRCryptoTransfer transfer) {
    return NULL == transfer->sourceAddress ? NULL : cryptoAddressTake (transfer->sourceAddress);
}

extern BRCryptoAddress
cryptoTransferGetTargetAddress (BRCryptoTransfer transfer) {
    return NULL == transfer->targetAddress ? NULL : cryptoAddressTake (transfer->targetAddress);
}

static BRCryptoAmount
cryptoTransferGetAmountAsSign (BRCryptoTransfer transfer, BRCryptoBoolean isNegative) {
    BRCryptoAmount   amount;

    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            uint64_t fee = transfer->u.btc.fee;
            if (UINT64_MAX == fee) fee = 0;

            uint64_t recv = transfer->u.btc.recv;
            uint64_t send = transfer->u.btc.send;

            switch (cryptoTransferGetDirection(transfer)) {
                case CRYPTO_TRANSFER_RECOVERED:
                    amount = cryptoAmountCreate (transfer->unit,
                                                 isNegative,
                                                 createUInt256(send));
                    break;

                case CRYPTO_TRANSFER_SENT:
                    amount = cryptoAmountCreate (transfer->unit,
                                               isNegative,
                                               createUInt256(send - fee - recv));
                    break;

                case CRYPTO_TRANSFER_RECEIVED:
                    amount = cryptoAmountCreate (transfer->unit,
                                               isNegative,
                                               createUInt256(recv));
                    break;
                    
                default: assert(0);
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumAmount ethAmount = transferGetAmount(transfer->u.eth.tid);
            switch (amountGetType(ethAmount)) {
                case AMOUNT_ETHER:
                    amount = cryptoAmountCreate (transfer->unit,
                                               isNegative,
                                               etherGetValue(amountGetEther(ethAmount), WEI));
                    break;

                case AMOUNT_TOKEN:
                    amount = cryptoAmountCreate (transfer->unit,
                                               isNegative,
                                               amountGetTokenQuantity(ethAmount).valueAsInteger);
                    break;

                default: assert(0);
            }
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = transfer->u.gen.gwm;
            BRGenericTransfer tid = transfer->u.gen.tid;

            amount = cryptoAmountCreate (transfer->unit,
                                       isNegative,
                                       gwmTransferGetAmount (gwm, tid));
            break;
        }
    }

    return amount;
}

extern BRCryptoAmount
cryptoTransferGetAmount (BRCryptoTransfer transfer) {
    return cryptoTransferGetAmountAsSign (transfer, CRYPTO_FALSE);
}

extern BRCryptoAmount
cryptoTransferGetAmountDirected (BRCryptoTransfer transfer) {
    BRCryptoAmount   amount;

    switch (cryptoTransferGetDirection(transfer)) {
        case CRYPTO_TRANSFER_RECOVERED: {
            amount = cryptoAmountCreate (transfer->unit,
                                         CRYPTO_FALSE,
                                         UINT256_ZERO);
            break;
        }

        case CRYPTO_TRANSFER_SENT: {
            amount = cryptoTransferGetAmountAsSign (transfer,
                                                    CRYPTO_TRUE);
            break;
        }

        case CRYPTO_TRANSFER_RECEIVED: {
            amount = cryptoTransferGetAmountAsSign (transfer,
                                                    CRYPTO_FALSE);
            break;
        }
        default: assert(0);
    }

    return amount;
}

extern BRCryptoUnit
cryptoTransferGetUnitForAmount (BRCryptoTransfer transfer) {
    return cryptoUnitTake (transfer->unit);
}

extern BRCryptoUnit
cryptoTransferGetUnitForFee (BRCryptoTransfer transfer) {
    return cryptoUnitTake (transfer->unitForFee);
}

/*
 extern BRCryptoAmount
cryptoTransferGetFee (BRCryptoTransfer transfer) { // Pass in 'currency' as blockchain baseUnit
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            uint64_t fee = transfer->u.btc.fee;
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
            BREthereumTransfer tid =transfer->u.eth.tid;
            int overflow = 0;

            BREthereumEther amount = transferGetFee (tid, &overflow);
            assert (0 == overflow);

            return cryptoAmountCreate (transfer->currency, CRYPTO_FALSE, amount.valueInWEI);
        }
        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = transfer->u.gen.gwm;
            BRGenericTransfer tid = transfer->u.gen.tid;

            return cryptoAmountCreate (transfer->currency,
                                       CRYPTO_FALSE,
                                       gwmTransferGetFee (gwm, tid));
        }
    }
}
*/

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
            uint64_t fee = transfer->u.btc.fee;
            if (UINT64_MAX == fee) fee = 0;

            uint64_t send = transfer->u.btc.send;
            uint64_t recv = transfer->u.btc.recv;

            if (0 == send) {
                return CRYPTO_TRANSFER_RECEIVED;
            } else if ((send - fee) == recv) {
                return CRYPTO_TRANSFER_RECOVERED;
            } else if ((send - fee) > recv) {
                return CRYPTO_TRANSFER_SENT;
            }

            return CRYPTO_TRANSFER_RECEIVED;
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumAddress source = transferGetSourceAddress (tid);
            BREthereumAddress target = transferGetTargetAddress (tid);

            BREthereumBoolean accountIsSource = addressEqual (source, transfer->u.eth.accountAddress);
            BREthereumBoolean accountIsTarget = addressEqual (target, transfer->u.eth.accountAddress);

            if (accountIsSource == ETHEREUM_BOOLEAN_TRUE && accountIsTarget == ETHEREUM_BOOLEAN_TRUE) {
                return CRYPTO_TRANSFER_RECOVERED;
            } else if (accountIsSource == ETHEREUM_BOOLEAN_TRUE && accountIsTarget == ETHEREUM_BOOLEAN_FALSE) {
                return CRYPTO_TRANSFER_SENT;
            } else if (accountIsSource == ETHEREUM_BOOLEAN_FALSE && accountIsTarget == ETHEREUM_BOOLEAN_TRUE) {
                return CRYPTO_TRANSFER_RECEIVED;
            }

            assert(0);
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = transfer->u.gen.gwm;
            BRGenericTransfer tid = transfer->u.gen.tid;

            BRGenericAddress source = gwmTransferGetSourceAddress (gwm, tid);
            BRGenericAddress target = gwmTransferGetTargetAddress (gwm, tid);

            BRGenericAddress address = gwmGetAccountAddress (gwm);

            int accountIsSource = gwmAddressEqual (gwm, source, address);
            int accountIsTarget = gwmAddressEqual (gwm, target, address);

            if      ( accountIsSource &&  accountIsTarget) return CRYPTO_TRANSFER_RECOVERED;
            else if ( accountIsSource && !accountIsTarget) return CRYPTO_TRANSFER_SENT;
            else if (!accountIsSource &&  accountIsTarget) return CRYPTO_TRANSFER_RECOVERED;

            assert (0);
        }
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
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumHash hash = transferGetOriginatingTransactionHash (tid);
            return (ETHEREUM_BOOLEAN_TRUE == hashEqual(hash, hashCreateEmpty())
                    ? NULL
                    : cryptoHashCreateAsETH (hash));
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = transfer->u.gen.gwm;
            BRGenericTransfer tid = transfer->u.gen.tid;

            BRGenericHash hash = gwmTransferGetHash (gwm, tid);
            return (genericHashIsEmpty (hash)
                    ? NULL
                    : cryptoHashCreateAsGEN (hash));
        }
    }
}

/*
extern BRCryptoFeeBasis
cryptoTransferGetEstimatedFeeBasis (BRCryptoTransfer transfer) {
    BRCryptoFeeBasis feeBasis;

    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRTransaction *tid = transfer->u.btc.tid;

            uint64_t fee = transfer->u.btc.fee;
            uint32_t feePerKB = DEFAULT_FEE_PER_KB;
            uint32_t sizeInByte = (uint32_t) BRTransactionVSize (tid);

            if (UINT64_MAX != fee) {
                // round to nearest satoshi per kb
                feePerKB = (uint32_t) (((1000 * fee) + (sizeInByte/2)) / sizeInByte);
            }

            feeBasis = cryptoFeeBasisCreateAsBTC (transfer->unitForFee, feePerKB, sizeInByte);
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumTransfer tid =transfer->u.eth.tid;

            BREthereumFeeBasis ethFeeBasis = transferGetFeeBasis (tid);
            BREthereumGas gas = feeBasisGetGasLimit (ethFeeBasis);
            BREthereumGasPrice gasPrice = feeBasisGetGasPrice (ethFeeBasis);

            feeBasis = cryptoFeeBasisCreateAsETH (transfer->unitForFee, gas, gasPrice);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = transfer->u.gen.gwm;
            BRGenericTransfer tid = transfer->u.gen.tid;

            BRGenericFeeBasis bid = gwmTransferGetFeeBasis (gwm, tid);
            feeBasis = cryptoFeeBasisCreateAsGEN (transfer->unitForFee, gwm, bid);
            break;
        }
    }

    return feeBasis;
}
*/

extern BRCryptoFeeBasis
cryptoTransferGetEstimatedFeeBasis (BRCryptoTransfer transfer) {
    return (NULL == transfer->feeBasisEstimated ? NULL : cryptoFeeBasisTake (transfer->feeBasisEstimated));
}

extern BRCryptoFeeBasis
cryptoTransferGetConfirmedFeeBasis (BRCryptoTransfer transfer) {
    return (NULL == transfer->feeBasisConfirmed ? NULL : cryptoFeeBasisTake (transfer->feeBasisConfirmed));
}

private_extern void
cryptoTransferSetConfirmedFeeBasis (BRCryptoTransfer transfer,
                                    BRCryptoFeeBasis feeBasisConfirmed) {
    BRCryptoFeeBasis takenFeeBasisConfirmed = (NULL == feeBasisConfirmed ? NULL : cryptoFeeBasisTake(feeBasisConfirmed));
    if (NULL != transfer->feeBasisConfirmed) cryptoFeeBasisGive (transfer->feeBasisConfirmed);
    transfer->feeBasisConfirmed = takenFeeBasisConfirmed;
}

private_extern BRTransaction *
cryptoTransferAsBTC (BRCryptoTransfer transfer) {
    assert (BLOCK_CHAIN_TYPE_BTC == transfer->type);
    return transfer->u.btc.tid;
}

private_extern BREthereumTransfer
cryptoTransferAsETH (BRCryptoTransfer transfer) {
    assert (BLOCK_CHAIN_TYPE_ETH == transfer->type);
    return transfer->u.eth.tid;
}

private_extern BRGenericTransfer
cryptoTransferAsGEN (BRCryptoTransfer transfer) {
    assert (BLOCK_CHAIN_TYPE_GEN == transfer->type);
    return transfer->u.gen.tid;
}

private_extern BRCryptoBoolean
cryptoTransferHasBTC (BRCryptoTransfer transfer,
                      BRTransaction *btc) {
    // Since BTC transactions that are not yet signed do not have a txHash, only
    // use the BRTransactionEq check when the transfer's transaction is signed.
    // If it is not signed, use an identity check.
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == transfer->type && (BRTransactionIsSigned (transfer->u.btc.tid)
                                                                         ? BRTransactionEq (btc, transfer->u.btc.tid)
                                                                         : (btc == transfer->u.btc.tid)));
}

private_extern BRCryptoBoolean
cryptoTransferHasETH (BRCryptoTransfer transfer,
                      BREthereumTransfer eth) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == transfer->type && eth == transfer->u.eth.tid);
}

private_extern BRCryptoBoolean
cryptoTransferHasGEN (BRCryptoTransfer transfer,
                      BRGenericTransfer gen) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_GEN == transfer->type && gen == transfer->u.gen.tid);
}

static int
cryptoTransferEqualAsBTC (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    // Since BTC transactions that are not yet signed do not have a txHash, only
    // use the BRTransactionEq check when at least one party is signed. For cases
    // where that is not true, use an identity check.
    return (BRTransactionIsSigned (t1->u.btc.tid)
            ? BRTransactionEq (t1->u.btc.tid, t2->u.btc.tid)
            : t1->u.btc.tid == t2->u.btc.tid);
}

static int
cryptoTransferEqualAsETH (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return t1->u.eth.tid == t2->u.eth.tid;
}

static int
cryptoTransferEqualAsGEN (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return (t1->u.gen.gwm == t2->u.gen.gwm &&
            t1->u.gen.tid == t2->u.gen.tid);
}

extern BRCryptoBoolean
cryptoTransferEqual (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return AS_CRYPTO_BOOLEAN (t1 == t2 || (t1->type == t2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == t1->type && cryptoTransferEqualAsBTC (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == t1->type && cryptoTransferEqualAsETH (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == t1->type && cryptoTransferEqualAsGEN (t1, t2)))));
}

