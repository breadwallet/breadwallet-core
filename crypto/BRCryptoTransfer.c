//
//  BRCryptoTransfer.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoTransferP.h"

#include "BRCryptoBase.h"
#include "BRCryptoHashP.h"
#include "BRCryptoAddressP.h"
#include "BRCryptoAmountP.h"
#include "BRCryptoFeeBasisP.h"

#include "support/BRAddress.h"
#include "bitcoin/BRWallet.h"
#include "bitcoin/BRTransaction.h"
#include "ethereum/BREthereum.h"

/// MARK: - Transfer State Type

extern const char *
cryptoTransferStateTypeString (BRCryptoTransferStateType type) {
    static const char *strings[] = {
        "CRYPTO_TRANSFER_STATE_CREATED",
        "CRYPTO_TRANSFER_STATE_SIGNED",
        "CRYPTO_TRANSFER_STATE_SUBMITTED",
        "CRYPTO_TRANSFER_STATE_INCLUDED",
        "CRYPTO_TRANSFER_STATE_ERRORED",
        "CRYPTO_TRANSFER_STATE_DELETED",
    };
    assert (CRYPTO_TRANSFER_EVENT_CREATED <= type && type <= CRYPTO_TRANSFER_STATE_DELETED);
    return strings[type];
}

/// MARK: Transfer

static BRCryptoTransferDirection
cryptoTransferDirectionFromBTC (uint64_t send, uint64_t recv, uint64_t fee);

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

    array_new (transfer->attributes, 1);

    transfer->ref = CRYPTO_REF_ASSIGN (cryptoTransferRelease);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

        pthread_mutex_init(&transfer->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsBTC (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           BRWallet *wid,
                           OwnershipKept BRTransaction *tid,
                           BRCryptoBoolean isBTC) {
    BRAddressParams  addressParams = BRWalletGetAddressParams (wid);
    BRCryptoTransfer transfer      = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_BTC, unit, unitForFee);
    transfer->u.btc.tid = tid;

    // cache the values that require the wallet
    transfer->u.btc.fee  = BRWalletFeeForTx (wid, tid);
    transfer->u.btc.recv = BRWalletAmountReceivedFromTx (wid, tid);
    transfer->u.btc.send = BRWalletAmountSentByTx (wid, tid);

    BRCryptoTransferDirection direction = cryptoTransferDirectionFromBTC (transfer->u.btc.send,
                                                                          transfer->u.btc.recv,
                                                                          transfer->u.btc.fee);

    {
        size_t     inputsCount = tid->inCount;
        BRTxInput *inputs      = tid->inputs;

        // If we receive the transfer, then we won't be the source address.
        int inputsContain = (CRYPTO_TRANSFER_RECEIVED != direction);

        for (size_t index = 0; index < inputsCount; index++) {
            size_t addressSize = BRTxInputAddress (&inputs[index], NULL, 0, addressParams);

            // ensure address fits in a BRAddress struct, which adds a nul-terminator
            assert (addressSize < sizeof (BRAddress));
            if (0 != addressSize && addressSize < sizeof (BRAddress)) {
                char address [addressSize + 1];
                BRTxInputAddress (&inputs[index], address, addressSize, addressParams);
                address [addressSize] = '\0'; // ensure address is nul-terminated

                if (inputsContain == BRWalletContainsAddress(wid, address)) {
                    transfer->sourceAddress = cryptoAddressCreateAsBTC (BRAddressFill (addressParams, address), isBTC);
                    break;
                }
            }
        }
    }

    {
        size_t      outputsCount = tid->outCount;
        BRTxOutput *outputs      = tid->outputs;

        // If we sent the transfer, then we won't be the target address.
        int outputsContain = (CRYPTO_TRANSFER_SENT != direction);

        for (size_t index = 0; index < outputsCount; index++) {
            size_t addressSize = BRTxOutputAddress (&outputs[index], NULL, 0, addressParams);

            // ensure address fits in a BRAddress struct, which adds a nul-terminator
            assert (addressSize < sizeof (BRAddress));
            if (0 != addressSize && addressSize < sizeof (BRAddress)) {
                // There will be no targetAddress if we send the amount to ourselves.  In that
                // case `outputsContain = 0` and every output is our own address and thus 1 is always
                // returned by `BRWalletContainsAddress()`
                char address [addressSize + 1];
                BRTxOutputAddress (&outputs[index], address, addressSize, addressParams);
                address [addressSize] = '\0'; // ensure address is nul-terminated

                if (outputsContain == BRWalletContainsAddress(wid, address)) {
                    transfer->targetAddress = cryptoAddressCreateAsBTC (BRAddressFill (addressParams, address), isBTC);
                    break;
                }
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
    transfer->u.eth.ewm = ewm;
    transfer->u.eth.tid = tid;

    transfer->sourceAddress = cryptoAddressCreateAsETH (ewmTransferGetSource (ewm, tid));
    transfer->targetAddress = cryptoAddressCreateAsETH (ewmTransferGetTarget (ewm, tid));

    // cache the values that require the ewm
    BREthereumAccount account = ewmGetAccount (ewm);
    transfer->u.eth.accountAddress = accountGetPrimaryAddress (account);

    // This function `cryptoTransferCreateAsETH()` includes an argument as
    // `BRCryptoFeeBasis feeBasisEstimated` whereas the analogous function
    // `cryptoTransferCreateAsBTC` does not.  Why is that?  For BTC the fee basis can be derived
    // 100% reliably from the BRTransaction; both the 'estimated' and 'confirmed' fee basises are
    // identical.  For ETH, the 'estimated' and the 'confirmed' basises may differ.  The difference
    // being the distinction between ETH `gasLimit` (the 'estimate') and `gasUsed` (the
    // 'confirmed').
    //
    // The EWM interface does not make this distinction clear.  It should.
    // TODO: In EWM expose 'getEstimatedFeeBasis' and 'getConfirmedFeeBasis' functions.
    //
    // Turns out that this function is called in two contexts - when Crypto creates a transfer (in
    // response to User input) and when EWM has a transfer announced (like when found in a
    // blockchain).  When Crypto creates the transfer we have the `feeBasisEstimated` and it is used
    // to create the EWM transfer.  Then EWM finds the transfer (see `cwmTransactionEventAsETH()`)
    // we don't have the estimated fee - if we did nothing the `transfer->feeBasisEstimated` field
    // would be NULL.
    //
    // Problem is we *require* one of 'estimated' or 'confirmed'.  See Transfer.swift at
    // `public var fee: Amount { ... guard let feeBasis = confirmedFeeBasis ?? estimatedFeeBasis }`
    // The 'confirmed' value is *ONLY SET* when a transfer is actually included in the blockchain;
    // therefore we need an estimated fee basis.
    //
    // Thus: if `feeBasisEstimated` is NULL, we'll take the ETH fee basis (as the best we have).

    // Get the ETH feeBasis, in the event that we need it.
    BREthereumFeeBasis ethFeeBasis = ewmTransferGetFeeBasis (ewm, tid);

    transfer->feeBasisEstimated = (NULL == feeBasisEstimated
                                   ? cryptoFeeBasisCreateAsETH (unitForFee,
                                                                ethFeeBasis.u.gas.limit,
                                                                ethFeeBasis.u.gas.price)
                                   : cryptoFeeBasisTake(feeBasisEstimated));

    return transfer;
}

extern BRCryptoTransfer
cryptoTransferCreateAsGEN (BRCryptoUnit unit,
                           BRCryptoUnit unitForFee,
                           OwnershipGiven BRGenericTransfer tid) {
    BRCryptoTransfer transfer = cryptoTransferCreateInternal (BLOCK_CHAIN_TYPE_GEN, unit, unitForFee);
    transfer->u.gen = tid;

    BRGenericFeeBasis gwmFeeBasis = genTransferGetFeeBasis (tid); // Will give ownership
    transfer->feeBasisEstimated = cryptoFeeBasisCreateAsGEN (transfer->unitForFee, gwmFeeBasis);

    transfer->sourceAddress = cryptoAddressCreateAsGEN (genTransferGetSourceAddress (tid));
    transfer->targetAddress = cryptoAddressCreateAsGEN (genTransferGetTargetAddress (tid));

    return transfer;
}

static void
cryptoTransferRelease (BRCryptoTransfer transfer) {
    if (NULL != transfer->sourceAddress) cryptoAddressGive (transfer->sourceAddress);
    if (NULL != transfer->targetAddress) cryptoAddressGive (transfer->targetAddress);
    cryptoUnitGive (transfer->unit);
    cryptoUnitGive (transfer->unitForFee);
    cryptoTransferStateRelease (&transfer->state);
    if (NULL != transfer->feeBasisEstimated) cryptoFeeBasisGive (transfer->feeBasisEstimated);

    array_free_all(transfer->attributes, cryptoTransferAttributeGive);
    
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;
        case BLOCK_CHAIN_TYPE_ETH:
            break;
        case BLOCK_CHAIN_TYPE_GEN:
            genTransferRelease(transfer->u.gen);
            break;
    }

    pthread_mutex_destroy (&transfer->lock);

    memset (transfer, 0, sizeof(*transfer));
    free (transfer);
}

private_extern BRCryptoBlockChainType
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
            BREthereumAmount ethAmount = ewmTransferGetAmount (transfer->u.eth.ewm,
                                                               transfer->u.eth.tid);
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
            BRGenericTransfer tid = transfer->u.gen;

            amount = cryptoAmountCreate (transfer->unit,
                                         isNegative,
                                         genTransferGetAmount (tid));
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

extern BRCryptoAmount
cryptoTransferGetAmountDirectedNet (BRCryptoTransfer transfer) {
    BRCryptoAmount amount = cryptoTransferGetAmountDirected (transfer);

    // If the transfer->unit and transfer->unitForFee differ then there is no fee
    if (cryptoUnitIsIdentical (transfer->unit, transfer->unitForFee))
        return amount;

    BRCryptoFeeBasis feeBasis = cryptoTransferGetConfirmedFeeBasis(transfer);
    if (NULL == feeBasis)
        feeBasis = (NULL == transfer->feeBasisEstimated
                    ? NULL
                    : cryptoFeeBasisTake (transfer->feeBasisEstimated));

    // If there is no fee basis, then there is no fee
    if (NULL == feeBasis)
        return amount;

    BRCryptoAmount fee = cryptoFeeBasisGetFee (feeBasis);
    cryptoFeeBasisGive(feeBasis);

    // Simply subtract off the fee.
    BRCryptoAmount amountNet = cryptoAmountSub (amount, fee);

    cryptoAmountGive(fee);
    cryptoAmountGive(amount);

    return amountNet;
}

extern BRCryptoUnit
cryptoTransferGetUnitForAmount (BRCryptoTransfer transfer) {
    return cryptoUnitTake (transfer->unit);
}

extern BRCryptoUnit
cryptoTransferGetUnitForFee (BRCryptoTransfer transfer) {
    return cryptoUnitTake (transfer->unitForFee);
}

extern size_t
cryptoTransferGetAttributeCount (BRCryptoTransfer transfer) {
    pthread_mutex_lock (&transfer->lock);
    size_t count = array_count(transfer->attributes);
    pthread_mutex_unlock (&transfer->lock);
    return count;
}

extern BRCryptoTransferAttribute
cryptoTransferGetAttributeAt (BRCryptoTransfer transfer,
                              size_t index) {
    pthread_mutex_lock (&transfer->lock);
    BRCryptoTransferAttribute attribute = cryptoTransferAttributeTake (transfer->attributes[index]);
    pthread_mutex_unlock (&transfer->lock);
    return attribute;
}

private_extern void
cryptoTransferSetAttributes (BRCryptoTransfer transfer,
                             OwnershipKept BRArrayOf(BRCryptoTransferAttribute) attributes) {
    pthread_mutex_lock (&transfer->lock);

    // Give existing attributes and empty `transfer->attributes`
    for (size_t index = 0; index < array_count(transfer->attributes); index++)
        cryptoTransferAttributeGive (transfer->attributes[index]);
    array_clear(transfer->attributes);

    if (NULL != attributes)
        // Take new attributes.
        for (size_t index = 0; index < array_count(attributes); index++)
            array_add (transfer->attributes, cryptoTransferAttributeTake (attributes[index]));
    pthread_mutex_unlock (&transfer->lock);
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
                                       genTransferGetFee (gwm, tid));
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

extern BRCryptoTransferStateType
cryptoTransferGetStateType (BRCryptoTransfer transfer) {
    return transfer->state.type;
}

extern BRCryptoTransferState
cryptoTransferGetState (BRCryptoTransfer transfer) {
    pthread_mutex_lock (&transfer->lock);
    BRCryptoTransferState state = cryptoTransferStateCopy (&transfer->state);
    pthread_mutex_unlock (&transfer->lock);

    return state;
}

private_extern void
cryptoTransferSetState (BRCryptoTransfer transfer,
                        BRCryptoTransferState state) {
    BRCryptoTransferState newState = cryptoTransferStateCopy (&state);

    pthread_mutex_lock (&transfer->lock);
    BRCryptoTransferState oldState = transfer->state;
    transfer->state = newState;
    pthread_mutex_unlock (&transfer->lock);

    cryptoTransferStateRelease (&oldState);
}

static BRCryptoTransferDirection
cryptoTransferDirectionFromBTC (uint64_t send, uint64_t recv, uint64_t fee) {
    if (UINT64_MAX == fee) fee = 0;

    return (0 == send
            ? CRYPTO_TRANSFER_RECEIVED
            : ((send - fee) == recv
               ? CRYPTO_TRANSFER_RECOVERED
               : ((send - fee) > recv
                  ? CRYPTO_TRANSFER_SENT
                  : CRYPTO_TRANSFER_RECEIVED)));
}

extern BRCryptoTransferDirection
cryptoTransferGetDirection (BRCryptoTransfer transfer) {
    switch (transfer->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return cryptoTransferDirectionFromBTC (transfer->u.btc.send,
                                                   transfer->u.btc.recv,
                                                   transfer->u.btc.fee);

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM      ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid = transfer->u.eth.tid;

            BREthereumAddress source = ewmTransferGetSource (ewm, tid);
            BREthereumAddress target = ewmTransferGetTarget (ewm, tid);

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

        case BLOCK_CHAIN_TYPE_GEN:
            switch (genTransferGetDirection (transfer->u.gen)) {
                case GENERIC_TRANSFER_SENT:      return CRYPTO_TRANSFER_SENT;
                case GENERIC_TRANSFER_RECEIVED:  return CRYPTO_TRANSFER_RECEIVED;
                case GENERIC_TRANSFER_RECOVERED: return CRYPTO_TRANSFER_RECOVERED;
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
            BREthereumEWM      ewm = transfer->u.eth.ewm;
            BREthereumTransfer tid = transfer->u.eth.tid;

            BREthereumHash hash = ewmTransferGetOriginatingTransactionHash (ewm, tid);
            return (ETHEREUM_BOOLEAN_TRUE == hashEqual(hash, hashCreateEmpty())
                    ? NULL
                    : cryptoHashCreateAsETH (hash));
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericTransfer tid = transfer->u.gen;

            BRGenericHash hash = genTransferGetHash (tid);
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

            BRGenericFeeBasis bid = genTransferGetFeeBasis (gwm, tid);
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
    pthread_mutex_lock (&transfer->lock);
    BRCryptoFeeBasis feeBasisConfirmed = (CRYPTO_TRANSFER_STATE_INCLUDED == transfer->state.type
                                          ? cryptoFeeBasisTake (transfer->state.u.included.feeBasis)
                                          : NULL);
    pthread_mutex_unlock (&transfer->lock);

    return feeBasisConfirmed;
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
    return transfer->u.gen;
}

private_extern BRCryptoBoolean
cryptoTransferHasBTC (BRCryptoTransfer transfer,
                      BRTransaction *btc) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_BTC == transfer->type && btc == transfer->u.btc.tid);
}

private_extern BRCryptoBoolean
cryptoTransferHasETH (BRCryptoTransfer transfer,
                      BREthereumTransfer eth) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_ETH == transfer->type && eth == transfer->u.eth.tid);
}

private_extern BRCryptoBoolean
cryptoTransferHasGEN (BRCryptoTransfer transfer,
                      BRGenericTransfer gen) {
    return AS_CRYPTO_BOOLEAN (BLOCK_CHAIN_TYPE_GEN == transfer->type &&
                              genTransferEqual (gen, transfer->u.gen));
}

static int
cryptoTransferEqualAsBTC (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    // This does not compare the properties of `t1` to `t2`, just the 'id-ness'.  If the properties
    // are compared, one needs to be careful about the BRTransaction's timestamp.  Two transactions
    // with an identical hash can have different timestamps depending on how the transaction
    // is identified.  Specifically P2P and API found transactions *will* have different timestamps.
    return t1->u.btc.tid == t2->u.btc.tid;
}

static int
cryptoTransferEqualAsETH (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return t1->u.eth.tid == t2->u.eth.tid;
}

static int
cryptoTransferEqualAsGEN (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return genTransferEqual (t1->u.gen, t2->u.gen);
}

extern BRCryptoBoolean
cryptoTransferEqual (BRCryptoTransfer t1, BRCryptoTransfer t2) {
    return AS_CRYPTO_BOOLEAN (t1 == t2 || (t1->type == t2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == t1->type && cryptoTransferEqualAsBTC (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == t1->type && cryptoTransferEqualAsETH (t1, t2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == t1->type && cryptoTransferEqualAsGEN (t1, t2)))));
}

extern BRCryptoComparison
cryptoTransferCompare (BRCryptoTransfer transfer1, BRCryptoTransfer transfer2) {
    // early bail when comparing the same transfer
    if (CRYPTO_TRUE == cryptoTransferEqual (transfer1, transfer2)) {
        return CRYPTO_COMPARE_EQ;
    }

    // The algorithm below is captured in the cryptoTransferCompare declaration
    // comments; any changes to this routine must be reflected in that comment
    // and vice versa).
    //
    // The algorithm includes timestamp as a differentiator despite the fact that
    // timestamp is likely derived from the block. Thus, an occurrence where timestamp
    // is different while block value is the same is unlikely. Regardless, this check
    // is included to handle cases where that assumption does not hold.
    //
    // Another reason to include timestamp is if this function were used to order
    // transfers across different wallets. While not anticipated to be a common use
    // case, there is not enough information available in the transfer object to
    // preclude it from happening. Checking on the `type` field is insufficient
    // given that GEN will handle multiple cases. While block number and transaction
    // index are meaningless comparables between wallets, ordering by timestamp
    // does provide some value.

    BRCryptoComparison compareValue;
    BRCryptoTransferState state1 = cryptoTransferGetState (transfer1);
    BRCryptoTransferState state2 = cryptoTransferGetState (transfer2);

    // neither transfer is included
    if (state1.type != CRYPTO_TRANSFER_STATE_INCLUDED &&
        state2.type != CRYPTO_TRANSFER_STATE_INCLUDED) {
        // we don't have anything to sort on other than identity
        compareValue = (uintptr_t) transfer1 > (uintptr_t) transfer2 ?
            CRYPTO_COMPARE_GT : CRYPTO_COMPARE_LT;

    // transfer1 is NOT included (and transfer2 is)
    } else if (state1.type != CRYPTO_TRANSFER_STATE_INCLUDED) {
        // return "greater than" for transfer1
        compareValue = CRYPTO_COMPARE_GT;

    // transfer2 is NOT included (and transfer1 is)
    } else if (state2.type != CRYPTO_TRANSFER_STATE_INCLUDED) {
        // return "lesser than" for transfer1
        compareValue = CRYPTO_COMPARE_LT;

    // both are included, check if the timestamp differs
    } else if (state1.u.included.timestamp != state2.u.included.timestamp) {
        // return based on the greater timestamp
        compareValue = state1.u.included.timestamp > state2.u.included.timestamp ?
            CRYPTO_COMPARE_GT : CRYPTO_COMPARE_LT;

    // both are included and have the same timestamp, check if the block differs
    } else if (state1.u.included.blockNumber != state2.u.included.blockNumber) {
        // return based on the greater block number
        compareValue = state1.u.included.blockNumber > state2.u.included.blockNumber ?
            CRYPTO_COMPARE_GT : CRYPTO_COMPARE_LT;

    // both are included and have the same timestamp and block, check if the index differs
    } else if (state1.u.included.transactionIndex != state2.u.included.transactionIndex) {
        // return based on the greater index
        compareValue = state1.u.included.transactionIndex > state2.u.included.transactionIndex ?
            CRYPTO_COMPARE_GT : CRYPTO_COMPARE_LT;

    // both are included and have the same timestamp, block and index
    } else {
        // we are out of differentiators, return "equal"
        compareValue = CRYPTO_COMPARE_EQ;
    }

    // clean up on the way out
    cryptoTransferStateRelease (&state1);
    cryptoTransferStateRelease (&state2);
    return compareValue;
}

extern void
cryptoTransferExtractBlobAsBTC (BRCryptoTransfer transfer,
                                uint8_t **bytes,
                                size_t   *bytesCount,
                                uint32_t *blockHeight,
                                uint32_t *timestamp) {
    assert (NULL != bytes && NULL != bytesCount);

    BRTransaction *tx = cryptoTransferAsBTC (transfer);

    *bytesCount = BRTransactionSerialize (tx, NULL, 0);
    *bytes = malloc (*bytesCount);
    BRTransactionSerialize (tx, *bytes, *bytesCount);

    if (NULL != blockHeight) *blockHeight = tx->blockHeight;
    if (NULL != timestamp)   *timestamp   = tx->timestamp;
}

extern BRCryptoTransferState
cryptoTransferStateInit (BRCryptoTransferStateType type) {
    switch (type) {
        case CRYPTO_TRANSFER_STATE_CREATED:
        case CRYPTO_TRANSFER_STATE_DELETED:
        case CRYPTO_TRANSFER_STATE_SIGNED:
        case CRYPTO_TRANSFER_STATE_SUBMITTED: {
            return (BRCryptoTransferState) {
                type
            };
        }
        case CRYPTO_TRANSFER_STATE_INCLUDED:
            assert (0); // if you are hitting this, use cryptoTransferStateIncludedInit!
            return (BRCryptoTransferState) {
                CRYPTO_TRANSFER_STATE_INCLUDED,
                { .included = { 0, 0, 0, NULL }}
            };
        case CRYPTO_TRANSFER_STATE_ERRORED: {
            assert (0); // if you are hitting this, use cryptoTransferStateErroredInit!
            return (BRCryptoTransferState) {
                CRYPTO_TRANSFER_STATE_ERRORED,
                { .errored = { cryptoTransferSubmitErrorUnknown() }}
            };
        }
    }
}

extern BRCryptoTransferState
cryptoTransferStateIncludedInit (uint64_t blockNumber,
                                 uint64_t transactionIndex,
                                 uint64_t timestamp,
                                 BRCryptoFeeBasis feeBasis,
                                 BRCryptoBoolean success,
                                 const char *error) {
    BRCryptoTransferState result = (BRCryptoTransferState) {
        CRYPTO_TRANSFER_STATE_INCLUDED,
        { .included = {
            blockNumber,
            transactionIndex,
            timestamp,
            cryptoFeeBasisTake(feeBasis),
            success
        }}
    };

    memset (result.u.included.error, 0, CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE + 1);
    if (CRYPTO_FALSE == success) {
        strlcpy (result.u.included.error,
                 (NULL == error ? "unknown error" : error),
                 CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE + 1);
    }

    return result;
}

extern BRCryptoTransferState
cryptoTransferStateErroredInit (BRCryptoTransferSubmitError error) {
    return (BRCryptoTransferState) {
        CRYPTO_TRANSFER_STATE_ERRORED,
        { .errored = { error }}
    };
}

extern BRCryptoTransferState
cryptoTransferStateCopy (BRCryptoTransferState *state) {
    BRCryptoTransferState newState = *state;
    switch (state->type) {
        case CRYPTO_TRANSFER_STATE_INCLUDED: {
            if (NULL != newState.u.included.feeBasis) {
                cryptoFeeBasisTake (newState.u.included.feeBasis);
            }
            break;
        }
        case CRYPTO_TRANSFER_STATE_ERRORED:
        case CRYPTO_TRANSFER_STATE_CREATED:
        case CRYPTO_TRANSFER_STATE_DELETED:
        case CRYPTO_TRANSFER_STATE_SIGNED:
        case CRYPTO_TRANSFER_STATE_SUBMITTED:
        default: {
            break;
        }
    }
    return newState;
}

extern void
cryptoTransferStateRelease (BRCryptoTransferState *state) {
    switch (state->type) {
        case CRYPTO_TRANSFER_STATE_INCLUDED: {
            if (NULL != state->u.included.feeBasis) {
                cryptoFeeBasisGive (state->u.included.feeBasis);
            }
            break;
        }
        case CRYPTO_TRANSFER_STATE_ERRORED:
        case CRYPTO_TRANSFER_STATE_CREATED:
        case CRYPTO_TRANSFER_STATE_DELETED:
        case CRYPTO_TRANSFER_STATE_SIGNED:
        case CRYPTO_TRANSFER_STATE_SUBMITTED:
        default: {
            break;
        }
    }

    memset (state, 0, sizeof(*state));
}

extern const char *
cryptoTransferEventTypeString (BRCryptoTransferEventType t) {
    switch (t) {
        case CRYPTO_TRANSFER_EVENT_CREATED:
        return "CRYPTO_TRANSFER_EVENT_CREATED";

        case CRYPTO_TRANSFER_EVENT_CHANGED:
        return "CRYPTO_TRANSFER_EVENT_CHANGED";

        case CRYPTO_TRANSFER_EVENT_DELETED:
        return "CRYPTO_TRANSFER_EVENT_DELETED";
    }
    return "<CRYPTO_TRANSFER_EVENT_TYPE_UNKNOWN>";
}


/// MARK: Transaction Submission Error

// TODO(fix): This should be moved to a more appropriate file (BRTransfer.c/h?)

extern BRCryptoTransferSubmitError
cryptoTransferSubmitErrorUnknown(void) {
    return (BRCryptoTransferSubmitError) {
        CRYPTO_TRANSFER_SUBMIT_ERROR_UNKNOWN
    };
}

extern BRCryptoTransferSubmitError
cryptoTransferSubmitErrorPosix(int errnum) {
    return (BRCryptoTransferSubmitError) {
        CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX,
        { .posix = { errnum } }
    };
}

extern char *
cryptoTransferSubmitErrorGetMessage (BRCryptoTransferSubmitError *e) {
    char *message = NULL;

    switch (e->type) {
        case CRYPTO_TRANSFER_SUBMIT_ERROR_POSIX: {
            if (NULL != (message = strerror (e->u.posix.errnum))) {
                message = strdup (message);
            }
            break;
        }
        default: {
            break;
        }
    }

    return message;
}


/// MARK: - Transfer Attribute

struct BRCryptoTransferAttributeRecord {
    char *key;
    char *value;
    BRCryptoBoolean isRequired;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoTransferAttribute, cryptoTransferAttribute)

private_extern BRCryptoTransferAttribute
cryptoTransferAttributeCreate (const char *key,
                               const char *val,
                               BRCryptoBoolean isRequired) {
    BRCryptoTransferAttribute attribute = calloc (1, sizeof (struct BRCryptoTransferAttributeRecord));

    attribute->key   = strdup (key);
    attribute->value = (NULL == val ? NULL : strdup (val));
    attribute->isRequired = isRequired;

    attribute->ref = CRYPTO_REF_ASSIGN (cryptoTransferAttributeRelease);

    return attribute;
}

extern BRCryptoTransferAttribute
cryptoTransferAttributeCopy (BRCryptoTransferAttribute attribute) {
    return cryptoTransferAttributeCreate (attribute->key,
                                          attribute->value,
                                          attribute->isRequired);
}

static void
cryptoTransferAttributeRelease (BRCryptoTransferAttribute attribute) {
    free (attribute->key);
    if (NULL != attribute->value) free (attribute->value);
    memset (attribute, 0, sizeof (struct BRCryptoTransferAttributeRecord));
    free (attribute);
}

extern const char *
cryptoTransferAttributeGetKey (BRCryptoTransferAttribute attribute) {
    return attribute->key;
}

extern const char * // nullable
cryptoTransferAttributeGetValue (BRCryptoTransferAttribute attribute) {
    return attribute->value;
}
extern void
cryptoTransferAttributeSetValue (BRCryptoTransferAttribute attribute, const char *value) {
    if (NULL != attribute->value) free (attribute->value);
    attribute->value = (NULL == value ? NULL : strdup (value));
}

extern BRCryptoBoolean
cryptoTransferAttributeIsRequired (BRCryptoTransferAttribute attribute) {
    return attribute->isRequired;
}

DECLARE_CRYPTO_GIVE_TAKE (BRCryptoTransferAttribute, cryptoTransferAttribute);
