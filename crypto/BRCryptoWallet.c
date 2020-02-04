//
//  BRCryptoWallet.c
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include "BRCryptoWalletP.h"

#include "BRCryptoFeeBasis.h"
#include "BRCryptoAmount.h"

#include "BRCryptoAmountP.h"
#include "BRCryptoKeyP.h"
#include "BRCryptoFeeBasisP.h"
#include "BRCryptoTransferP.h"
#include "BRCryptoAddressP.h"
#include "BRCryptoNetworkP.h"
#include "BRCryptoPaymentP.h"

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
                         OwnershipKept BRGenericWallet wid) {
    BRCryptoWallet wallet = cryptoWalletCreateInternal (BLOCK_CHAIN_TYPE_GEN, unit, unitForFee);

    wallet->u.gen = wid;

    return wallet;
}

static void
cryptoWalletRelease (BRCryptoWallet wallet) {
    cryptoUnitGive (wallet->unit);
    cryptoUnitGive(wallet->unitForFee);

    for (size_t index = 0; index < array_count(wallet->transfers); index++)
        cryptoTransferGive (wallet->transfers[index]);
    array_free (wallet->transfers);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            break;

        case BLOCK_CHAIN_TYPE_ETH:
            break;

        case BLOCK_CHAIN_TYPE_GEN:
            // wallet->u.gen is owned by the GEN manager
            break;
    }
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
            return cryptoAmountCreate (wallet->unit,
                                       CRYPTO_FALSE,
                                       genWalletGetBalance (cryptoWalletAsGEN(wallet)));
        }
    }
}

extern BRCryptoAmount /* nullable */
cryptoWalletGetBalanceMinimum (BRCryptoWallet wallet) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, createUInt256(0));

        case BLOCK_CHAIN_TYPE_ETH:
            return cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, createUInt256(0));

        case BLOCK_CHAIN_TYPE_GEN: {
            BRCryptoBoolean hasLimit = 0;
            UInt256 limit = genWalletGetBalanceLimit (cryptoWalletAsGEN(wallet), CRYPTO_FALSE, &hasLimit);
            return (CRYPTO_FALSE == hasLimit
                    ? NULL
                    : cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, limit));
        }
    }
}

extern BRCryptoAmount /* nullable */
cryptoWalletGetBalanceMaximum (BRCryptoWallet wallet) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            return NULL;

        case BLOCK_CHAIN_TYPE_ETH:
            return NULL;

        case BLOCK_CHAIN_TYPE_GEN: {
            BRCryptoBoolean hasLimit = 0;
            UInt256 limit = genWalletGetBalanceLimit (cryptoWalletAsGEN(wallet), CRYPTO_TRUE, &hasLimit);
            return (CRYPTO_FALSE == hasLimit
                    ? NULL
                    : cryptoAmountCreate (wallet->unit, CRYPTO_FALSE, limit));
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

extern void
cryptoWalletAddTransfer (BRCryptoWallet wallet,
                         BRCryptoTransfer transfer) {
    pthread_mutex_lock (&wallet->lock);
    if (CRYPTO_FALSE == cryptoWalletHasTransfer (wallet, transfer)) {
        array_add (wallet->transfers, cryptoTransferTake(transfer));
    }
    pthread_mutex_unlock (&wallet->lock);
}

extern void
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
            BRGenericWallet wid = wallet->u.gen;

            BRGenericAddress genAddress = genWalletGetAddress (wid);
            return cryptoAddressCreateAsGEN (genAddress);
        }
    }
}

extern BRCryptoBoolean
cryptoWalletHasAddress (BRCryptoWallet wallet,
                        BRCryptoAddress address) {
    if (wallet->type != cryptoAddressGetType(address))
        return CRYPTO_FALSE;

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRCryptoBoolean isBitcoinAddress;

            BRWallet *btcWallet = wallet->u.btc.wid;
            BRAddress btcAddress = cryptoAddressAsBTC (address, &isBitcoinAddress);

            if (BRWalletAddressIsUsed (btcWallet, btcAddress.s))
                return CRYPTO_TRUE;

            BRAddress btcLegacyAddress = BRWalletLegacyAddress (btcWallet);
            if (0 == memcmp (btcAddress.s, btcLegacyAddress.s, sizeof (btcAddress.s)))
                return CRYPTO_TRUE;

            if (CRYPTO_TRUE == isBitcoinAddress) {
                BRAddress btcSegwitAddress = BRWalletReceiveAddress (btcWallet);
                if (0 == memcmp (btcAddress.s, btcSegwitAddress.s, sizeof (btcAddress.s)))
                    return CRYPTO_TRUE;
            }

            return CRYPTO_FALSE;
        }

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumAddress ethAddress = cryptoAddressAsETH (address);
            return AS_CRYPTO_BOOLEAN (ETHEREUM_BOOLEAN_TRUE == ewmWalletHasAddress(wallet->u.eth.ewm, wallet->u.eth.wid, ethAddress));
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            return AS_CRYPTO_BOOLEAN (genWalletHasAddress (wallet->u.gen, address->u.gen));
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
            feeBasis = cryptoFeeBasisCreateAsBTC (feeUnit, (uint32_t) BRWalletFeePerKb (wid), 1000);
            break;
        }
        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;

            BREthereumGas gas = ewmWalletGetDefaultGasLimit (ewm, wid);
            BREthereumGasPrice gasPrice = ewmWalletGetDefaultGasPrice (ewm, wid);

            feeBasis =  cryptoFeeBasisCreateAsETH (feeUnit, gas, gasPrice);
            break;
        }
        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet wid = wallet->u.gen;

            BRGenericFeeBasis bid = genWalletGetDefaultFeeBasis (wid);
            feeBasis =  cryptoFeeBasisCreateAsGEN (feeUnit, bid);
            break;
        }
    }

    cryptoUnitGive (feeUnit);

    return feeBasis;
}

extern void
cryptoWalletSetDefaultFeeBasis (BRCryptoWallet wallet,
                                BRCryptoFeeBasis feeBasis) {
    assert (cryptoWalletGetType(wallet) == cryptoFeeBasisGetType (feeBasis));

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            // This will generate a BTC BITCOIN_WALLET_PER_PER_KB_UPDATED event.
             BRWalletManagerUpdateFeePerKB (wallet->u.btc.bwm,
                                            wallet->u.btc.wid,
                                            cryptoFeeBasisAsBTC(feeBasis));
            break;

        case BLOCK_CHAIN_TYPE_ETH: {
            BREthereumEWM ewm = wallet->u.eth.ewm;
            BREthereumWallet wid =wallet->u.eth.wid;
            BREthereumFeeBasis ethFeeBasis = cryptoFeeBasisAsETH (feeBasis);

            // These will generate EWM WALLET_EVENT_DEFAULT_GAS_{LIMIT,PRICE}_UPDATED events
            ewmWalletSetDefaultGasLimit (ewm, wid, ethFeeBasis.u.gas.limit);
            ewmWalletSetDefaultGasPrice (ewm, wid, ethFeeBasis.u.gas.price);
            break;
        }

        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericWallet wid = wallet->u.gen;

            // This will not generate any GEN events (GEN events don't exist);
            genWalletSetDefaultFeeBasis (wid, cryptoFeeBasisAsGEN(feeBasis));
            break;
        }
    }
}

extern size_t
cryptoWalletGetTransferAttributeCount (BRCryptoWallet wallet,
                                       BRCryptoAddress target) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: return 0;
        case BLOCK_CHAIN_TYPE_ETH: return 0;
        case BLOCK_CHAIN_TYPE_GEN: {
            return genWalletGetTransferAttributeCount (wallet->u.gen, (NULL == target ? NULL : target->u.gen));
        }
    }
}

extern BRCryptoTransferAttribute
cryptoWalletGetTransferAttributeAt (BRCryptoWallet wallet,
                                    BRCryptoAddress target,
                                    size_t index) {
    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: return NULL;
        case BLOCK_CHAIN_TYPE_ETH: return NULL;
        case BLOCK_CHAIN_TYPE_GEN: {
            BRGenericTransferAttribute attribute = genWalletGetTransferAttributeAt (wallet->u.gen, (NULL == target ? NULL : target->u.gen), index);
            return cryptoTransferAttributeCreate (genTransferAttributeGetKey(attribute),
                                                  genTransferAttributeGetVal(attribute),  // Expect NULL from genWalletGetTransferAttributeAt()
                                                  AS_CRYPTO_BOOLEAN(genTransferAttributeIsRequired(attribute)));
        }
    }
}

extern BRCryptoTransferAttributeValidationError
cryptoWalletValidateTransferAttribute (BRCryptoWallet wallet,
                                       OwnershipKept BRCryptoTransferAttribute attribute,
                                       BRCryptoBoolean *validates) {
    int ignore = 0;
    *validates = CRYPTO_TRUE;

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: return (BRCryptoTransferAttributeValidationError) ignore;
        case BLOCK_CHAIN_TYPE_ETH: return (BRCryptoTransferAttributeValidationError) ignore;
        case BLOCK_CHAIN_TYPE_GEN: {
            if (CRYPTO_TRUE == cryptoTransferAttributeIsRequired(attribute) && NULL == cryptoTransferAttributeGetValue(attribute)) {
                *validates = CRYPTO_FALSE;
                return CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_REQUIRED_BUT_NOT_PROVIDED;
            }

            BRGenericTransferAttribute genAttribute =
            genTransferAttributeCreate (cryptoTransferAttributeGetKey(attribute),
                                        cryptoTransferAttributeGetValue(attribute),
                                        cryptoTransferAttributeIsRequired(attribute));

            *validates = genWalletValidateTransferAttribute(wallet->u.gen, genAttribute);
            genTransferAttributeRelease (genAttribute);
            return (CRYPTO_FALSE == *validates
                    ? CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_MISMATCHED_TYPE
                    : (BRCryptoTransferAttributeValidationError) ignore);
        }
    }
}

extern BRCryptoTransferAttributeValidationError
cryptoWalletValidateTransferAttributes (BRCryptoWallet wallet,
                                        size_t attributesCount,
                                        OwnershipKept BRCryptoTransferAttribute *attributes,
                                        BRCryptoBoolean *validates) {
    int ignore = 0;
    *validates = CRYPTO_TRUE;

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: return (BRCryptoTransferAttributeValidationError) ignore;
        case BLOCK_CHAIN_TYPE_ETH: return (BRCryptoTransferAttributeValidationError) ignore;
        case BLOCK_CHAIN_TYPE_GEN: {
            // Check if all required attributes are provided and have values

            // If we've no attributes (and have no required attributes), success.
            if (0 == attributesCount) return (BRCryptoTransferAttributeValidationError) ignore;

            // Check individual validity
            for (size_t index = 0; index < attributesCount; index++) {
                BRCryptoTransferAttributeValidationError error = cryptoWalletValidateTransferAttribute (wallet, attributes[index], validates);
                if (CRYPTO_FALSE == *validates) return error;
            }

            // Check joint validity
            BRArrayOf(BRGenericTransferAttribute) genAttributes;
            array_new (genAttributes, attributesCount);
            for (size_t index = 0; index < attributesCount; index++) {
                BRCryptoTransferAttribute attribute = attributes[index];
                BRGenericTransferAttribute genAttribute =
                genTransferAttributeCreate(cryptoTransferAttributeGetKey(attribute),
                                         cryptoTransferAttributeGetValue(attribute),
                                         cryptoTransferAttributeIsRequired(attribute));
                array_add (genAttributes, genAttribute);
            }

            *validates = genWalletValidateTransferAttributes (wallet->u.gen, genAttributes);
            genTransferAttributeReleaseAll(genAttributes);

            return (CRYPTO_FALSE == *validates
                    ? CRYPTO_TRANSFER_ATTRIBUTE_VALIDATION_ERROR_RELATIONSHIP_INCONSISTENCY
                    : (BRCryptoTransferAttributeValidationError) ignore);
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
    return wallet->u.gen;
}

extern BRCryptoTransfer
cryptoWalletCreateTransfer (BRCryptoWallet  wallet,
                            BRCryptoAddress target,
                            BRCryptoAmount  amount,
                            BRCryptoFeeBasis estimatedFeeBasis,
                            size_t attributesCount,
                            OwnershipKept BRCryptoTransferAttribute *attributes) {
    assert (cryptoWalletGetType(wallet) == cryptoAddressGetType(target));
    assert (cryptoWalletGetType(wallet) == cryptoFeeBasisGetType(estimatedFeeBasis));

    BRCryptoTransfer transfer;

    BRCryptoUnit unit       = cryptoWalletGetUnit (wallet);
    BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

    BRCryptoCurrency currency = cryptoUnitGetCurrency(unit);
    assert (cryptoAmountHasCurrency (amount, currency));
    cryptoCurrencyGive(currency);

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
            BRGenericWallet wid = wallet->u.gen;

            UInt256 genValue  = cryptoAmountGetValue (amount);
            BRGenericAddress genAddr = cryptoAddressAsGEN (target);
            BRGenericFeeBasis genFeeBasis = cryptoFeeBasisAsGEN (estimatedFeeBasis);

            // The GEN Wallet is holding the `tid` memory
            BRGenericTransfer tid = NULL;

            if (0 == attributesCount)
                tid = genWalletCreateTransfer (wid, genAddr, genValue, genFeeBasis);
            else {
                BRArrayOf(BRGenericTransferAttribute) genAttributes;
                array_new (genAttributes, attributesCount);
                for (size_t index = 0; index < attributesCount; index++) {
                    // There is no need to give/take this attribute.  It is OwnershipKept
                    // (by the caller) and we only extract info.
                    BRCryptoTransferAttribute attribute = attributes[index];
                    BRGenericTransferAttribute genAttribute =
                    genTransferAttributeCreate(cryptoTransferAttributeGetKey(attribute),
                                             cryptoTransferAttributeGetValue(attribute),
                                             cryptoTransferAttributeIsRequired(attribute));
                    array_add (genAttributes, genAttribute);
                }
                tid = genWalletCreateTransferWithAttributes (wid, genAddr, genValue, genFeeBasis, genAttributes);
                genTransferAttributeReleaseAll(genAttributes);
            }

            // The CRYPTO Transfer holds the `tid` memory (w/ REF count of 1)
            transfer = NULL == tid ? NULL : cryptoTransferCreateAsGEN (unit, unitForFee, tid);
            break;
        }
    }

    if (NULL != transfer && attributesCount > 0) {
        BRArrayOf (BRCryptoTransferAttribute) transferAttributes;
        array_new (transferAttributes, attributesCount);
        array_add_array (transferAttributes, attributes, attributesCount);
        cryptoTransferSetAttributes (transfer, transferAttributes);
        array_free (transferAttributes);
    }

    cryptoUnitGive (unitForFee);
    cryptoUnitGive (unit);

    return transfer;
}

extern BRCryptoTransfer
cryptoWalletCreateTransferForWalletSweep (BRCryptoWallet  wallet,
                                          BRCryptoWalletSweeper sweeper,
                                          BRCryptoFeeBasis estimatedFeeBasis) {
    BRCryptoTransfer transfer = NULL;

    BRCryptoUnit unit       = cryptoWalletGetUnit (wallet);
    BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = wallet->u.btc.bwm;
            BRWallet *wid = wallet->u.btc.wid;

            BRTransaction *tid = BRWalletManagerCreateTransactionForSweep (bwm,
                                                                           wid,
                                                                           cryptoWalletSweeperAsBTC(sweeper),
                                                                           cryptoFeeBasisAsBTC(estimatedFeeBasis));
            transfer = NULL == tid ? NULL : cryptoTransferCreateAsBTC (unit,
                                                                       unitForFee,
                                                                       wid,
                                                                       tid,
                                                                       AS_CRYPTO_BOOLEAN(BRWalletManagerHandlesBTC(bwm)));
            break;
        }
        default:
            assert (0);
            break;
    }

    cryptoUnitGive (unitForFee);
    cryptoUnitGive (unit);

    return transfer;
}

extern BRCryptoTransfer
cryptoWalletCreateTransferForPaymentProtocolRequest (BRCryptoWallet wallet,
                                                     BRCryptoPaymentProtocolRequest request,
                                                     BRCryptoFeeBasis estimatedFeeBasis) {
    BRCryptoTransfer transfer = NULL;

    BRCryptoUnit unit       = cryptoWalletGetUnit (wallet);
    BRCryptoUnit unitForFee = cryptoWalletGetUnitForFee(wallet);

    switch (wallet->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWalletManager bwm = wallet->u.btc.bwm;
            BRWallet *wid = wallet->u.btc.wid;

            switch (cryptoPaymentProtocolRequestGetType (request)) {
                case CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY:
                case CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70: {
                    BRArrayOf(BRTxOutput) outputs = cryptoPaymentProtocolRequestGetOutputsAsBTC (request);
                    if (NULL != outputs) {
                        BRTransaction *tid = BRWalletManagerCreateTransactionForOutputs (bwm, wid, outputs, array_count (outputs),
                                                                                         cryptoFeeBasisAsBTC(estimatedFeeBasis));
                        transfer = NULL == tid ? NULL : cryptoTransferCreateAsBTC (unit, unitForFee, wid, tid,
                                                                                   AS_CRYPTO_BOOLEAN(BRWalletManagerHandlesBTC(bwm)));
                        array_free (outputs);
                    }
                    break;
                }
                default: {
                    assert (0);
                    break;
                }
            }
            break;
        }
        default: {
            assert (0);
            break;
        }
    }

    cryptoUnitGive (unitForFee);
    cryptoUnitGive (unit);

    return transfer;
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
            return cryptoFeeBasisCreateAsGEN (wallet->unitForFee,
                                              (BRGenericFeeBasis) {
                value,
                costFactor
            });
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
    return (w1->u.gen == w2->u.gen);
}

extern BRCryptoBoolean
cryptoWalletEqual (BRCryptoWallet w1, BRCryptoWallet w2) {
    return AS_CRYPTO_BOOLEAN (w1 == w2 || (w1->type == w2->type &&
                                           ((BLOCK_CHAIN_TYPE_BTC == w1->type && cryptoWalletEqualAsBTC (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_ETH == w1->type && cryptoWalletEqualAsETH (w1, w2)) ||
                                            (BLOCK_CHAIN_TYPE_GEN == w1->type && cryptoWalletEqualAsGEN (w1, w2)))));
}

extern const char *
cryptoWalletEventTypeString (BRCryptoWalletEventType t) {
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

/// MARK: Wallet Sweeper

struct BRCryptoWalletSweeperRecord {
    BRCryptoBlockChainType type;
    BRCryptoKey key;
    BRCryptoUnit unit;
    union {
        struct {
            BRWalletSweeper sweeper;
        } btc;
    } u;
    ;
};

static BRCryptoWalletSweeperStatus
BRWalletSweeperStatusToCrypto (BRWalletSweeperStatus t) {
    switch (t) {
        case WALLET_SWEEPER_SUCCESS: return CRYPTO_WALLET_SWEEPER_SUCCESS;
        case WALLET_SWEEPER_INVALID_TRANSACTION: return CRYPTO_WALLET_SWEEPER_INVALID_TRANSACTION;
        case WALLET_SWEEPER_INVALID_SOURCE_WALLET: return CRYPTO_WALLET_SWEEPER_INVALID_SOURCE_WALLET;
        case WALLET_SWEEPER_NO_TRANSACTIONS_FOUND: return CRYPTO_WALLET_SWEEPER_NO_TRANSFERS_FOUND;
        case WALLET_SWEEPER_INSUFFICIENT_FUNDS: return CRYPTO_WALLET_SWEEPER_INSUFFICIENT_FUNDS;
        case WALLET_SWEEPER_UNABLE_TO_SWEEP: return CRYPTO_WALLET_SWEEPER_UNABLE_TO_SWEEP;
    }
}

extern BRCryptoWalletSweeperStatus
cryptoWalletSweeperValidateSupported (BRCryptoNetwork network,
                                      BRCryptoCurrency currency,
                                      BRCryptoKey key,
                                      BRCryptoWallet wallet) {
    if (CRYPTO_FALSE == cryptoNetworkHasCurrency (network, currency)) {
        return CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS;
    }

    if (cryptoNetworkGetType (network) != cryptoWalletGetType (wallet)) {
        return CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS;
    }

    BRCryptoCurrency walletCurrency = cryptoWalletGetCurrency (wallet);
    if (CRYPTO_FALSE == cryptoCurrencyIsIdentical (currency, walletCurrency)) {
        cryptoCurrencyGive (walletCurrency);
        return CRYPTO_WALLET_SWEEPER_INVALID_ARGUMENTS;
    }
    cryptoCurrencyGive (walletCurrency);

    if (CRYPTO_FALSE == cryptoKeyHasSecret (key)) {
        return CRYPTO_WALLET_SWEEPER_INVALID_KEY;
    }

    switch (cryptoWalletGetType (wallet)) {
        case BLOCK_CHAIN_TYPE_BTC: {
            BRWallet * wid             = cryptoWalletAsBTC (wallet);
            BRKey * keyCore            = cryptoKeyGetCore (key);
            BRAddressParams addrParams = cryptoNetworkAsBTC (network)->addrParams;

            return BRWalletSweeperStatusToCrypto (BRWalletSweeperValidateSupported (keyCore,
                                                                                    addrParams,
                                                                                    wid));
        }
        default:{
            break;
        }
    }

    return CRYPTO_WALLET_SWEEPER_UNSUPPORTED_CURRENCY;
}

extern BRCryptoWalletSweeper
cryptoWalletSweeperCreateAsBtc (BRCryptoNetwork network,
                                BRCryptoCurrency currency,
                                BRCryptoKey key,
                                BRCryptoAddressScheme scheme) {
    assert (cryptoKeyHasSecret (key));
    BRCryptoWalletSweeper sweeper = calloc (1, sizeof(struct BRCryptoWalletSweeperRecord));
    sweeper->type = BLOCK_CHAIN_TYPE_BTC;
    sweeper->key = cryptoKeyTake (key);
    sweeper->unit = cryptoNetworkGetUnitAsBase (network, currency);
    sweeper->u.btc.sweeper = BRWalletSweeperNew(cryptoKeyGetCore (key),
                                                cryptoNetworkAsBTC (network)->addrParams,
                                                CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT == scheme);
    return sweeper;
}

extern void
cryptoWalletSweeperRelease (BRCryptoWalletSweeper sweeper) {
    switch (sweeper->type) {
        case BLOCK_CHAIN_TYPE_BTC:
            BRWalletSweeperFree (sweeper->u.btc.sweeper);
            break;
        default:
            assert (0);
            break;
    }
    cryptoKeyGive (sweeper->key);
    cryptoUnitGive (sweeper->unit);

    memset (sweeper, 0, sizeof(struct BRCryptoWalletSweeperRecord));
    free (sweeper);
}

extern BRCryptoWalletSweeperStatus
cryptoWalletSweeperHandleTransactionAsBTC (BRCryptoWalletSweeper sweeper,
                                           OwnershipKept uint8_t *transaction,
                                           size_t transactionLen) {
    BRCryptoWalletSweeperStatus status = CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION;

    switch (sweeper->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            status = BRWalletSweeperStatusToCrypto (
                BRWalletSweeperHandleTransaction (sweeper->u.btc.sweeper,
                                                  transaction, transactionLen)
            );
            break;
        }
        default:
            assert (0);
            break;
    }

    return status;
}

extern BRCryptoKey
cryptoWalletSweeperGetKey (BRCryptoWalletSweeper sweeper) {
    return cryptoKeyTake (sweeper->key);
}

extern char *
cryptoWalletSweeperGetAddress (BRCryptoWalletSweeper sweeper) {
    char * address = NULL;

    switch (sweeper->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            address = BRWalletSweeperGetLegacyAddress (sweeper->u.btc.sweeper);
            break;
        }
        default:
            assert (0);
            break;
    }

    return address;
}

extern BRCryptoAmount
cryptoWalletSweeperGetBalance (BRCryptoWalletSweeper sweeper) {
    BRCryptoAmount amount = NULL;

    switch (sweeper->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            UInt256 value = createUInt256 (BRWalletSweeperGetBalance (sweeper->u.btc.sweeper));
            amount = cryptoAmountCreate (sweeper->unit, CRYPTO_FALSE, value);
            break;
        }
        default:
            assert (0);
            break;
    }

    return amount;
}

extern BRCryptoWalletSweeperStatus
cryptoWalletSweeperValidate (BRCryptoWalletSweeper sweeper) {
    BRCryptoWalletSweeperStatus status = CRYPTO_WALLET_SWEEPER_ILLEGAL_OPERATION;

    switch (sweeper->type) {
        case BLOCK_CHAIN_TYPE_BTC: {
            status = BRWalletSweeperStatusToCrypto (
                BRWalletSweeperValidate(sweeper->u.btc.sweeper)
            );
            break;
        }
        default:
            assert (0);
            break;
    }

    return status;
}

private_extern BRWalletSweeper
cryptoWalletSweeperAsBTC (BRCryptoWalletSweeper sweeper) {
    assert (BLOCK_CHAIN_TYPE_BTC == sweeper->type);
    return sweeper->u.btc.sweeper;
}
