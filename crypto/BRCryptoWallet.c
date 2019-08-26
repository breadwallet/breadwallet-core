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

#include <pthread.h>

#include "BRCryptoFeeBasis.h"
#include "BRCryptoWallet.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "generic/BRGeneric.h"

#include "bitcoin/BRWallet.h"
#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"
#include "ethereum/ewm/BREthereumTransfer.h"

/**
 *
 */
static void
cryptoWalletRelease (BRCryptoWallet wallet);


struct BRCryptoWalletRecord {
    pthread_mutex_t lock;

    BRCryptoBlockChainType type;
    union {
        struct {
            BRWalletManager bwm;
            BRWallet *wid;
        } btc;

        struct {
            BREthereumEWM ewm;
            BREthereumWallet wid;
        } eth;

        struct {
            BRGenericWalletManager gwm;
            BRGenericWallet wid;
        } gen;
    } u;

    BRCryptoWalletState state;
    BRCryptoUnit unit;  // baseUnit

    //
    // Do we hold transfers here?  The BRWallet and the BREthereumWallet already hold transfers.
    // Shouldn't we defer to those to get transfers (and then wrap them in BRCryptoTransfer)?
    // Then we avoid caching trouble (in part).  For a newly created transaction (not yet signed),
    // the BRWallet will not hold a BRTransaction however, BREthereumWallet will hold a new
    // BREthereumTransaction. From BRWalet: `assert(tx != NULL && BRTransactionIsSigned(tx));`
    //
    // We are going to have the same
    //
    BRArrayOf (BRCryptoTransfer) transfers;

    //
    BRCryptoUnit unitForFee;
    BRCryptoRef ref;
};

IMPLEMENT_CRYPTO_GIVE_TAKE (BRCryptoWallet, cryptoWallet)

static BRCryptoWallet
cryptoWalletCreateInternal (BRCryptoBlockChainType type,
                            BRCryptoUnit unit,
                            BRCryptoUnit unitForFee) {
    BRCryptoWallet wallet = malloc (sizeof (struct BRCryptoWalletRecord));

    wallet->type  = type;
    wallet->state = CRYPTO_WALLET_STATE_CREATED;
    wallet->unit  = cryptoUnitTake (unit);
    wallet->unitForFee = cryptoUnitTake (unitForFee);
    array_new (wallet->transfers, 5);

    wallet->ref = CRYPTO_REF_ASSIGN (cryptoWalletRelease);

    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

        pthread_mutex_init(&wallet->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    return wallet;
}

private_extern BRCryptoWallet
cryptoWalletCreateAsBTC (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BRWalletManager bwm,
                         BRWallet *wid) {
    BRCryptoWallet wallet = cryptoWalletCreateInternal (BLOCK_CHAIN_TYPE_BTC, unit, unitForFee);

    wallet->u.btc.bwm = bwm;
    wallet->u.btc.wid = wid;

    return wallet;
}

private_extern BRCryptoWallet
cryptoWalletCreateAsETH (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BREthereumEWM ewm,
                         BREthereumWallet wid) {
    BRCryptoWallet wallet = cryptoWalletCreateInternal (BLOCK_CHAIN_TYPE_ETH, unit, unitForFee);

    wallet->u.eth.ewm = ewm;
    wallet->u.eth.wid = wid;

    return wallet;
}

private_extern BRCryptoWallet
cryptoWalletCreateAsGEN (BRCryptoUnit unit,
                         BRCryptoUnit unitForFee,
                         BRGenericWalletManager gwm,
                         BRGenericWallet wid) {
    BRCryptoWallet wallet = cryptoWalletCreateInternal (BLOCK_CHAIN_TYPE_GEN, unit, unitForFee);

    wallet->u.gen.gwm = gwm;
    wallet->u.gen.wid = wid;

    return wallet;
}

static void
cryptoWalletRelease (BRCryptoWallet wallet) {
    printf ("Wallet: Release\n");
    cryptoUnitGive (wallet->unit);
    cryptoUnitGive(wallet->unitForFee);

    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        cryptoTransferGive (wallet->transfers[index]);
    array_free (wallet->transfers);

    pthread_mutex_destroy (&wallet->lock);

    memset (wallet, 0, sizeof(*wallet));
    free (wallet);
}

private_extern BRCryptoBlockChainType
cryptoWalletGetType (BRCryptoWallet wallet) {
    return wallet->type;
}

extern BRCryptoCurrency
cryptoWalletGetCurrency (BRCryptoWallet wallet) {
    return cryptoUnitGetCurrency(wallet->unit);
}

extern BRCryptoUnit
cryptoWalletGetUnit (BRCryptoWallet wallet) {
    return cryptoUnitTake (wallet->unit);
}

extern BRCryptoWalletState
cryptoWalletGetState (BRCryptoWallet wallet) {
    return wallet->state;
}

private_extern void
cryptoWalletSetState (BRCryptoWallet wallet,
                      BRCryptoWalletState state) {
    wallet->state = state;
}

extern BRCryptoCurrency
cryptoWalletGetCurrencyForFee (BRCryptoWallet wallet) {
    return cryptoUnitGetCurrency (wallet->unitForFee);
}

extern BRCryptoUnit
cryptoWalletGetUnitForFee (BRCryptoWallet wallet) {
    return cryptoUnitTake (wallet->unitForFee);
}

extern BRCryptoAmount
cryptoWalletGetBalance (BRCryptoWallet wallet) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            UInt256 value = createUInt256 (BRWalletBalance (wid));
            BRCryptoAmount amount = cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, value);
            return amount;
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;

            BREthereumAmount balance = ewmWalletGetBalance (ewm, wid);
            UInt256 value = balance.type == AMOUNT_ETHER ? balance.u.ether.valueInWEI : balance.u.tokenQuantity.valueAsInteger;
            BRCryptoAmount amount = cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, value);
            return amount;
        }
        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            BRGenericWallet wid = wallet->u.gen.wid;

            // TODO: How does the GEN wallet know the balance?  Holds tranactions? (not currently)
            UInt256 value = gwmWalletGetBalance (gwm, wid);
            BRCryptoAmount amount = cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, value);
            return amount;
        }
    }
}

extern BRCryptoBoolean
cryptoWalletHasTransfer (BRCryptoWallet wallet,
                         BRCryptoTransfer transfer) {
    BRCryptoBoolean r = CRYPTO_FALSE;
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers) && CRYPTO_FALSE == r; index++) {
        r = cryptoTransferEqual (transfer, wallet->transfers[index]);
    }
    pthread_mutex_unlock (&wallet->lock);
    return r;
}

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsBTC (BRCryptoWallet wallet,
                               BRTransaction *btc) {
    BRCryptoTransfer transfer = NULL;
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers); index++) {
        if (CRYPTO_TRUE == cryptoTransferHasBTC (wallet->transfers[index], btc)) {
            transfer = cryptoTransferTake (wallet->transfers[index]);
            break;
        }
    }
    pthread_mutex_unlock (&wallet->lock);
    return transfer;
}

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsETH (BRCryptoWallet wallet,
                               BREthereumTransfer eth) {
    BRCryptoTransfer transfer = NULL;
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers); index++) {
        if (CRYPTO_TRUE == cryptoTransferHasETH (wallet->transfers[index], eth)) {
            transfer = cryptoTransferTake (wallet->transfers[index]);
            break;
        }
    }
    pthread_mutex_unlock (&wallet->lock);
    return transfer;
}

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsGEN (BRCryptoWallet wallet,
                               BRGenericTransfer gen) {
    BRCryptoTransfer transfer = NULL;
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers); index++) {
        if (CRYPTO_TRUE == cryptoTransferHasGEN (wallet->transfers[index], gen)) {
            transfer = cryptoTransferTake (wallet->transfers[index]);
            break;
        }
    }
    pthread_mutex_unlock (&wallet->lock);
    return transfer;
}

private_extern void
cryptoWalletAddTransfer (BRCryptoWallet wallet,
                         BRCryptoTransfer transfer) {
    pthread_mutex_lock (&wallet->lock);
    if (CRYPTO_FALSE == cryptoWalletHasTransfer (wallet, transfer)) {
        array_add (wallet->transfers, cryptoTransferTake(transfer));
    }
    pthread_mutex_unlock (&wallet->lock);
}

private_extern void
cryptoWalletRemTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer) {
    BRCryptoTransfer walletTransfer = NULL;
    pthread_mutex_lock (&wallet->lock);
    for (size_t index = 0; index < array_count(wallet->transfers); index++) {
        if (CRYPTO_TRUE == cryptoTransferEqual (wallet->transfers[index], transfer)) {
            walletTransfer = wallet->transfers[index];
            array_rm (wallet->transfers, index);
            break;
        }
    }
    pthread_mutex_unlock (&wallet->lock);

    // drop reference outside of lock to avoid potential case where release function runs
    if (NULL != walletTransfer) cryptoTransferGive (transfer);
}

extern BRCryptoTransfer *
cryptoWalletGetTransfers (BRCryptoWallet wallet, size_t *count) {
    pthread_mutex_lock (&wallet->lock);
    *count = array_count (wallet->transfers);
    BRCryptoTransfer *transfers = NULL;
    if (0 != *count) {
        transfers = calloc (*count, sizeof(BRCryptoTransfer));
        for (size_t index = 0; index < *count; index++) {
            transfers[index] = cryptoTransferTake(wallet->transfers[index]);
        }
    }
    pthread_mutex_unlock (&wallet->lock);
    return transfers;
}

//
// Returns a 'new' adddress.  For BTC this is a segwit/bech32 address.  Really needs to be a
// wallet configuration parameters (aka the 'address scheme')
//
extern BRCryptoAddress
cryptoWalletGetAddress (BRCryptoWallet wallet,
                        BRCryptoAddressScheme addressScheme) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            assert (CRYPTO_ADDRESS_SCHEME_BTC_LEGACY == addressScheme ||
                    CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT == addressScheme);

            BRWallet *wid = wallet->u.btc.wid;

            BRAddress btcAddress = (CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT == addressScheme
                                    ? BRWalletReceiveAddress(wid)
                                    : BRWalletLegacyAddress (wid));
            return cryptoAddressCreateAsBTC (btcAddress, AS_CRYPTO_BOOLEAN (BRWalletManagerHandlesBTC(wallet->u.btc.bwm)));
            }

        case BLOCK_CHAIN_TYPE_ETH: {
            assert (CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT == addressScheme);
            BREthereumEWM ewm = wallet->u.eth.ewm;

            BREthereumAddress ethAddress = accountGetPrimaryAddress (ewmGetAccount(ewm));
            return cryptoAddressCreateAsETH (ethAddress);
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            assert (CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT == addressScheme);
            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            BRGenericWallet wid = wallet->u.gen.wid;

            BRGenericAddress genAddress = gwmWalletGetAddress (gwm, wid);
            return cryptoAddressCreateAsGEN (gwm, genAddress);
        }
    }
}

extern BRCryptoFeeBasis
cryptoWalletGetDefaultFeeBasis (BRCryptoWallet wallet) {
    BRCryptoFeeBasis feeBasis;

    BRCryptoUnit feeUnit = cryptoWalletGetUnitForFee (wallet);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            assert (0); // TODO: Generic Size not 1000
            feeBasis = cryptoFeeBasisCreateAsBTC (feeUnit, BRWalletFeePerKb (wid), 1000);
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;

            BREthereumGas gas = ewmWalletGetDefaultGasLimit (ewm, wid);
            BREthereumGasPrice gasPrice = ewmWalletGetDefaultGasPrice (ewm, wid);

            feeBasis =  cryptoFeeBasisCreateAsETH (feeUnit, gas, gasPrice);
        }
        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            BRGenericWallet wid = wallet->u.gen.wid;

            BRGenericFeeBasis bid = gwmWalletGetDefaultFeeBasis (gwm, wid);
            feeBasis =  cryptoFeeBasisCreateAsGEN (feeUnit, gwm, bid);
        }
    }

    cryptoUnitGive (feeUnit);

    return feeBasis;
}

static void
cryptoWalletSetDefaultFeeBasisAsETH (BRCryptoWallet wallet,
                                     BREthereumFeeBasis ethFeeBasis) {
    BREthereumEWM ewm = wallet->u.eth.ewm;
    BREthereumWallet wid =wallet->u.eth.wid;

    // These will generate EWM WALLET_EVENT_DEFAULT_GAS_{LIMIT,PRICE}_UPDATED events
    ewmWalletSetDefaultGasLimit (ewm, wid, ethFeeBasis.u.gas.limit);
    ewmWalletSetDefaultGasPrice (ewm, wid, ethFeeBasis.u.gas.price);
}

static void
cryptoWalletSetDefaultFeeBasisAsBTC (BRCryptoWallet wallet,
                                     uint64_t btcFeeBasis) { // SAT-per-KB
    // This will generate a BTC BITCOIN_WALLET_PER_PER_KB_UPDATED event.
    BRWalletManagerUpdateFeePerKB (wallet->u.btc.bwm, wallet->u.btc.wid, btcFeeBasis);
}

extern void
cryptoWalletSetDefaultFeeBasis (BRCryptoWallet wallet,
                                BRCryptoFeeBasis feeBasis) {
    assert (cryptoWalletGetType(wallet) == cryptoFeeBasisGetType (feeBasis));

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            cryptoWalletSetDefaultFeeBasisAsBTC (wallet, cryptoFeeBasisAsBTC(feeBasis));
            break;

        case BLOCK_CHAIN_TYPE_ETH: {
            cryptoWalletSetDefaultFeeBasisAsETH (wallet, cryptoFeeBasisAsETH (feeBasis));
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            BRGenericWallet wid = wallet->u.gen.wid;

            gwmWalletSetDefaultFeeBasis (gwm, wid, cryptoFeeBasisAsGEN(feeBasis));
            break;
        }
    }
}

private_extern BRWallet *
cryptoWalletAsBTC (BRCryptoWallet wallet) {
    assert (BLOCK_CHAIN_TYPE_BTC == wallet->type);
    return wallet->u.btc.wid;
}

private_extern BREthereumWallet
cryptoWalletAsETH (BRCryptoWallet wallet) {
    assert (BLOCK_CHAIN_TYPE_ETH == wallet->type);
    return wallet->u.eth.wid;
}

private_extern BRGenericWallet
cryptoWalletAsGEN (BRCryptoWallet wallet) {
    assert (BLOCK_CHAIN_TYPE_GEN == wallet->type);
    return wallet->u.gen.wid;
}

extern BRCryptoTransfer
cryptoWalletCreateTransfer (BRCryptoWallet  wallet,
                            BRCryptoAddress target,
                            BRCryptoAmount  amount,
                            BRCryptoFeeBasis estimatedFeeBasis) {
    BRCryptoTransfer transfer;

    BRCryptoUnit unit       = cryptoWalletGetUnit (wallet);
    BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = wallet->u.btc.bwm;
            BRWallet *wid = wallet->u.btc.wid;

            BRCryptoBoolean isBitcoinAddr = CRYPTO_TRUE;
            BRAddress address = cryptoAddressAsBTC (target, &isBitcoinAddr);
            assert (isBitcoinAddr == AS_CRYPTO_BOOLEAN (BRWalletManagerHandlesBTC (bwm)));

            BRCryptoBoolean overflow = CRYPTO_FALSE;
            uint64_t value = cryptoAmountGetIntegerRaw (amount, &overflow);
            if (CRYPTO_TRUE == overflow) { return NULL; }

            BRTransaction *tid = BRWalletManagerCreateTransaction (bwm, wid, value, address,
                                                                   cryptoFeeBasisAsBTC(estimatedFeeBasis));
            transfer = NULL == tid ? NULL : cryptoTransferCreateAsBTC (unit, unitForFee, wid, tid,
                                                                       AS_CRYPTO_BOOLEAN(BRWalletManagerHandlesBTC(bwm)));
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid = wallet->u.eth.wid;

            UInt256 ethValue  = cryptoAmountGetValue (amount);
            BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wid);
            BREthereumAmount ethAmount = (NULL != ethToken
                                          ? amountCreateToken (createTokenQuantity (ethToken, ethValue))
                                          : amountCreateEther (etherCreate (ethValue)));
            BREthereumFeeBasis ethFeeBasis = cryptoFeeBasisAsETH (estimatedFeeBasis);

            char *addr = cryptoAddressAsString(target); // Target address

            //
            // We have a race condition here. `ewmWalletCreateTransferWithFeeBasis()` will generate
            // a `TRANSFER_EVENT_CREATED` event; `cwmTransactionEventAsETH()` will eventually get
            // called and attempt to find or create the BRCryptoTransfer.
            //
            // We might think about locking the wallet around the following two statements and then
            // locking in `cwmTransactionEventAsETH()` too - perhaps justifying it with 'we are
            // mucking w/ the wallet's transactions' so we should lock it (over a block of
            // statements).
            //
            BREthereumTransfer tid = ewmWalletCreateTransferWithFeeBasis (ewm, wid, addr, ethAmount, ethFeeBasis);
            transfer = NULL == tid ? NULL : cryptoTransferCreateAsETH (unit, unitForFee, ewm, tid, estimatedFeeBasis);

            free (addr);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            BRGenericWallet wid = wallet->u.gen.wid;

            UInt256 genValue  = cryptoAmountGetValue (amount);
            BRGenericAddress genAddr = cryptoAddressAsGEN (target);

            BRGenericTransfer tid = gwmWalletCreateTransfer (gwm, wid, genAddr, genValue);
            transfer = NULL == tid ? NULL : cryptoTransferCreateAsGEN (unit, unitForFee, gwm, tid);
            break;
        }
    }

    cryptoUnitGive (unitForFee);
    cryptoUnitGive (unit);

    return transfer;
}

extern void
cryptoWalletEstimateFeeBasis (BRCryptoWallet  wallet,
                              BRCryptoCookie cookie,
                              BRCryptoAddress target,
                              BRCryptoAmount  amount,
                              BRCryptoNetworkFee fee) {
    //    assert (cryptoWalletGetType (wallet) == cryptoFeeBasisGetType(feeBasis));
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = wallet->u.btc.bwm;
            BRWallet *wid = wallet->u.btc.wid;

            BRCryptoBoolean overflow = CRYPTO_FALSE;
            uint64_t feePerKB        = 1000 * cryptoNetworkFeeAsBTC (fee);
            uint64_t btcAmount       = cryptoAmountGetIntegerRaw (amount, &overflow);
            assert(CRYPTO_FALSE == overflow);

            BRWalletManagerEstimateFeeForTransfer (bwm,
                                                   wid,
                                                   cookie,
                                                   btcAmount,
                                                   feePerKB);
            break;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid = wallet->u.eth.wid;

            BRCryptoAddress source = cryptoWalletGetAddress (wallet, CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT);
            UInt256 ethValue       = cryptoAmountGetValue (amount);

            BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wid);
            BREthereumAmount ethAmount = (NULL != ethToken
                                    ? amountCreateToken (createTokenQuantity (ethToken, ethValue))
                                    : amountCreateEther (etherCreate (ethValue)));

            ewmWalletEstimateTransferFeeForTransfer (ewm,
                                                     wid,
                                                     cookie,
                                                     cryptoAddressAsETH (source),
                                                     cryptoAddressAsETH (target),
                                                     ethAmount,
                                                     cryptoNetworkFeeAsETH (fee),
                                                     ewmWalletGetDefaultGasLimit (ewm, wid));

            cryptoAddressGive (source);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            //            BRGenericWalletManager gwm = wallet->u.gen.gwm;
            //            BRGenericWallet wid = wallet->u.gen.wid;
            //
            //            UInt256 genValue = cryptoAmountGetValue(amount);

            // TODO: Generic EstimateFee
            assert (0);
            //            BRGenericFeeBasis genFeeBasis = cryptoFeeBasisAsGEN (feeBasis);
            //
            //            int overflow = 0;
            //            UInt256 genFee = gwmWalletEstimateTransferFee (gwm, wid, genValue, genFeeBasis, &overflow);
            //            assert (!overflow);
            //
            //            feeValue = genFee;
        }
    }
}

extern BRCryptoFeeBasis
cryptoWalletCreateFeeBasis (BRCryptoWallet wallet,
                            BRCryptoAmount pricePerCostFactor,
                            double costFactor) {

    BRCryptoCurrency feeCurrency = cryptoUnitGetCurrency (wallet->unitForFee);
    if (CRYPTO_FALSE == cryptoAmountHasCurrency (pricePerCostFactor, feeCurrency)) {
        cryptoCurrencyGive (feeCurrency);
        return NULL;
    }
    cryptoCurrencyGive (feeCurrency);

    UInt256 value = cryptoAmountGetValue (pricePerCostFactor);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            uint32_t feePerKB = value.u32[0];

            // Expect all other fields in `value` to be zero
            value.u32[0] = 0;
            if (!eqUInt256 (value, UINT256_ZERO)) return NULL;

            uint32_t sizeInBytes = (uint32_t) (1000 * costFactor);

            return cryptoFeeBasisCreateAsBTC (wallet->unitForFee, feePerKB, sizeInBytes);
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumGas gas = gasCreate((uint64_t) costFactor);
            BREthereumGasPrice gasPrice = gasPriceCreate (etherCreate(value));

            return cryptoFeeBasisCreateAsETH (wallet->unitForFee, gas, gasPrice);
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericFeeBasis feeBasis = NULL;
            return cryptoFeeBasisCreateAsGEN (wallet->unitForFee, wallet->u.gen.gwm, feeBasis);
        }
    }
}

static int
cryptoWalletEqualAsBTC (BRCryptoWallet w1, BRCryptoWallet w2) {
    return (w1->u.btc.bwm == w2->u.btc.bwm &&
            w1->u.btc.wid == w2->u.btc.wid);
}

static int
cryptoWalletEqualAsETH (BRCryptoWallet w1, BRCryptoWallet w2) {
    return (w1->u.eth.ewm == w2->u.eth.ewm &&
            w1->u.eth.wid == w2->u.eth.wid);
}

static int
cryptoWalletEqualAsGEN (BRCryptoWallet w1, BRCryptoWallet w2) {
    return (w1->u.gen.gwm == w2->u.gen.gwm &&
            w1->u.gen.wid == w2->u.gen.wid);
}

extern BRCryptoBoolean
cryptoWalletEqual (BRCryptoWallet w1, BRCryptoWallet w2) {
    return AS_CRYPTO_BOOLEAN (w1 == w2 || (w1->type == w2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == w1->type && cryptoWalletEqualAsBTC (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == w1->type && cryptoWalletEqualAsETH (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == w1->type && cryptoWalletEqualAsGEN (w1, w2)))));
}

extern const char *
BRCryptoWalletEventTypeString (BRCryptoWalletEventType t) {
    switch (t) {
        case CRYPTO_WALLET_EVENT_CREATED:
        return "CRYPTO_WALLET_EVENT_CREATED";

        case CRYPTO_WALLET_EVENT_CHANGED:
        return "CRYPTO_WALLET_EVENT_CHANGED";

        case CRYPTO_WALLET_EVENT_DELETED:
        return "CRYPTO_WALLET_EVENT_DELETED";

        case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
        return "CRYPTO_WALLET_EVENT_TRANSFER_ADDED";

        case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
        return "CRYPTO_WALLET_EVENT_TRANSFER_CHANGED";

        case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
        return "CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED";

        case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
        return "CRYPTO_WALLET_EVENT_TRANSFER_DELETED";

        case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
        return "CRYPTO_WALLET_EVENT_BALANCE_UPDATED";

        case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
        return "CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED";

        case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED:
        return "CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED";
    }
    return "<CRYPTO_WALLET_EVENT_TYPE_UNKNOWN>";
}
