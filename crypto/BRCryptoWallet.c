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

#include "BRCryptoFeeBasis.h"
#include "BRCryptoWallet.h"
#include "BRCryptoBase.h"
#include "BRCryptoPrivate.h"

#include "bitcoin/BRWallet.h"
#include "bitcoin/BRWalletManager.h"
#include "ethereum/BREthereum.h"

/**
 *
 */
static void
cryptoWalletRelease (BRCryptoWallet wallet);


struct BRCryptoWalletRecord {
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

static void
cryptoWalletRelease (BRCryptoWallet wallet) {
    printf ("Wallet: Release\n");
    cryptoUnitGive (wallet->unit);
    cryptoUnitGive(wallet->unitForFee);

    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        cryptoTransferGive (wallet->transfers[index]);
    array_free (wallet->transfers);

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
            BRCryptoCurrency currency = cryptoWalletGetCurrency (wallet);
            BRCryptoAmount amount = cryptoAmountCreate (currency, CRYPTO_FALSE, value);
            cryptoCurrencyGive (currency);
            return amount;
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;

            BREthereumAmount balance = ewmWalletGetBalance (ewm, wid);
            UInt256 value = balance.type == AMOUNT_ETHER ? balance.u.ether.valueInWEI : balance.u.tokenQuantity.valueAsInteger;
            BRCryptoCurrency currency = cryptoWalletGetCurrency (wallet);
            BRCryptoAmount amount = cryptoAmountCreate (currency, CRYPTO_FALSE, value);
            cryptoCurrencyGive (currency);
            return amount;
        }
        case BLOCK_CHAIN_TYPE_GEN:
            assert (0);
            return NULL;
    }
}

extern BRCryptoBoolean
cryptoWalletHasTransfer (BRCryptoWallet wallet,
                        BRCryptoTransfer transfer) {
    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        if (CRYPTO_TRUE == cryptoTransferEqual (transfer, wallet->transfers[index]))
            return CRYPTO_TRUE;
    return CRYPTO_FALSE;
}

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsBTC (BRCryptoWallet wallet,
                               BRTransaction *btc) {
    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        if (CRYPTO_TRUE == cryptoTransferHasBTC (wallet->transfers[index], btc))
            return cryptoTransferTake (wallet->transfers[index]);
    return NULL;
}

private_extern BRCryptoTransfer
cryptoWalletFindTransferAsETH (BRCryptoWallet wallet,
                               BREthereumTransfer eth) {
    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        if (CRYPTO_TRUE == cryptoTransferHasETH (wallet->transfers[index], eth))
            return cryptoTransferTake (wallet->transfers[index]);
    return NULL;
}

private_extern void
cryptoWalletAddTransfer (BRCryptoWallet wallet,
                         BRCryptoTransfer transfer) {
    if (CRYPTO_FALSE == cryptoWalletHasTransfer (wallet, transfer))
        array_add (wallet->transfers, cryptoTransferTake(transfer));
}

private_extern void
cryptoWalletRemTransfer (BRCryptoWallet wallet, BRCryptoTransfer transfer) {
    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        if (CRYPTO_TRUE == cryptoTransferEqual (wallet->transfers[index], transfer)) {
            array_rm (wallet->transfers, index);
            cryptoTransferGive (transfer);
            return;
        }
}

extern size_t
cryptoWalletGetTransferCount (BRCryptoWallet wallet) {
    return array_count (wallet->transfers);
}

extern BRCryptoTransfer
cryptoWalletGetTransfer (BRCryptoWallet wallet, size_t index) {
    return cryptoTransferTake (wallet->transfers[index]);
}

//
// Returns a 'new' adddress.  For BTC this is a segwit/bech32 address.  Really needs to be a
// wallet configuration parameters (aka the 'address scheme')
//
extern BRCryptoAddress
cryptoWalletGetAddress (BRCryptoWallet wallet) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            BRAddress btcAddress = BRWalletReceiveAddress(wid);
            return cryptoAddressCreateAsBTC (btcAddress);
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;

            BREthereumAddress ethAddress = accountGetPrimaryAddress (ewmGetAccount(ewm));
            return cryptoAddressCreateAsETH (ethAddress);
        }
        case BLOCK_CHAIN_TYPE_GEN:
            assert (0);
            return NULL;
    }
}

extern BRCryptoFeeBasis
cryptoWalletGetDefaultFeeBasis (BRCryptoWallet wallet) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            return cryptoFeeBasisCreateAsBTC (BRWalletFeePerKb (wid));
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;

            BREthereumGas gas = ewmWalletGetDefaultGasLimit (ewm, wid);
            BREthereumGasPrice gasPrice = ewmWalletGetDefaultGasPrice (ewm, wid);

            return cryptoFeeBasisCreateAsETH (gas, gasPrice);
        }
        case BLOCK_CHAIN_TYPE_GEN:
            assert (0);
            return NULL;
    }
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
                                     uint64_t btcFeeBasis) {
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

        case BLOCK_CHAIN_TYPE_ETH:
            cryptoWalletSetDefaultFeeBasisAsETH (wallet, cryptoFeeBasisAsETH (feeBasis));
            break;

        case BLOCK_CHAIN_TYPE_GEN:
            break;
    }
}

private_extern BRWallet *
cryptoWalletAsBTC (BRCryptoWallet wallet) {
    assert (BLOCK_CHAIN_TYPE_BTC ==  wallet->type);
    return wallet->u.btc.wid;
}

private_extern BREthereumWallet
cryptoWalletAsETH (BRCryptoWallet wallet) {
    assert (BLOCK_CHAIN_TYPE_ETH ==  wallet->type);
    return wallet->u.eth.wid;
}

extern BRCryptoTransfer
cryptoWalletCreateTransfer (BRCryptoWallet wallet,
                            BRCryptoAddress target,
                            BRCryptoAmount amount,
                            BRCryptoFeeBasis feeBasis) {
    assert (cryptoWalletGetType (wallet) == cryptoFeeBasisGetType(feeBasis));
    char *addr = cryptoAddressAsString(target); // Target address

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = wallet->u.btc.bwm;
            BRWallet *wid = wallet->u.btc.wid;

            BRCryptoBoolean overflow = CRYPTO_FALSE;
            uint64_t value = cryptoAmountGetIntegerRaw (amount, &overflow);
            if (CRYPTO_TRUE == overflow) { return NULL; }

            BRTransaction *tid = BRWalletManagerCreateTransaction (bwm, wid, value, addr);
            return NULL == tid ? NULL : cryptoTransferCreateAsBTC (cryptoWalletGetCurrency(wallet), wid, tid);
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid = wallet->u.eth.wid;

            UInt256 ethValue  = cryptoAmountGetValue (amount);
            BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wid);
            BREthereumAmount ethAmount = (NULL != ethToken
                                          ? amountCreateToken (createTokenQuantity (ethToken, ethValue))
                                          : amountCreateEther (etherCreate (ethValue)));
            BREthereumFeeBasis ethFeeBasis = cryptoFeeBasisAsETH (feeBasis);

            BREthereumTransfer tid = ewmWalletCreateTransferWithFeeBasis (ewm, wid, addr, ethAmount, ethFeeBasis);
            return NULL == tid ? NULL : cryptoTransferCreateAsETH (cryptoWalletGetCurrency(wallet), ewm, tid);
        }

        case BLOCK_CHAIN_TYPE_GEN:
            return NULL;
    }
}

extern BRCryptoAmount
cryptoWalletEstimateFee (BRCryptoWallet wallet,
                         BRCryptoAmount amount,
                         BRCryptoFeeBasis feeBasis,
                         BRCryptoUnit feeUnit) {
    assert (cryptoWalletGetType (wallet) == cryptoFeeBasisGetType(feeBasis));

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet *wid = wallet->u.btc.wid;

            uint64_t feePerKBSaved = BRWalletFeePerKb (wid);
            uint64_t feePerKB      = cryptoFeeBasisAsBTC (feeBasis);

            BRWalletSetFeePerKb (wid, feePerKB);
            BRCryptoBoolean overflow = CRYPTO_FALSE;
            uint64_t fee = BRWalletFeeForTxAmount (wid, cryptoAmountGetIntegerRaw (amount, &overflow));
            BRWalletSetFeePerKb (wid, feePerKBSaved);

            assert (CRYPTO_FALSE == overflow);
            return cryptoAmountCreateInteger (fee, feeUnit);
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid = wallet->u.eth.wid;

            int overflow = 0;
            UInt256 ethValue  = cryptoAmountGetValue (amount);
            BREthereumToken  ethToken  = ewmWalletGetToken (ewm, wid);
            BREthereumAmount ethAmount = (NULL != ethToken
                                          ? amountCreateToken (createTokenQuantity (ethToken, ethValue))
                                          : amountCreateEther (etherCreate (ethValue)));

            BREthereumFeeBasis ethFeeBasis = cryptoFeeBasisAsETH (feeBasis);
            BREthereumEther ethFee = ewmWalletEstimateTransferFeeForBasis (ewm, wid, ethAmount, ethFeeBasis.u.gas.price, ethFeeBasis.u.gas.limit, &overflow);

            assert (!overflow);
            return cryptoAmountCreateInternal (cryptoUnitGetCurrency(feeUnit), CRYPTO_FALSE, ethFee.valueInWEI, 0);
        }

        case BLOCK_CHAIN_TYPE_GEN:
            assert (0);
            return NULL;
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

extern BRCryptoBoolean
cryptoWalletEqual (BRCryptoWallet w1, BRCryptoWallet w2) {
    return AS_CRYPTO_BOOLEAN (w1 == w2 || (w1->type == w2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == w1->type && cryptoWalletEqualAsBTC (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == w1->type && cryptoWalletEqualAsETH (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == w1->type && 0))));
}
