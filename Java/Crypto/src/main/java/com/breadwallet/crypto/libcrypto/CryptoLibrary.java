/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.libcrypto;

import com.breadwallet.crypto.libcrypto.bitcoin.BRChainParams;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAccount;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWallet;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManager;
import com.breadwallet.crypto.libcrypto.bitcoin.BRWalletManagerClient;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAddress;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoAmount;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoCurrency;
import com.breadwallet.crypto.libcrypto.crypto.BRCryptoUnit;
import com.breadwallet.crypto.libcrypto.support.BRAddress;
import com.breadwallet.crypto.libcrypto.bitcoin.BRTransaction;
import com.breadwallet.crypto.libcrypto.ethereum.BREthereumAddress;
import com.breadwallet.crypto.libcrypto.support.BRMasterPubKey;
import com.breadwallet.crypto.libcrypto.support.UInt256;
import com.breadwallet.crypto.libcrypto.support.UInt512;
import com.breadwallet.crypto.libcrypto.utility.SizeT;
import com.breadwallet.crypto.libcrypto.utility.SizeTByReference;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.IntByReference;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

    // bitcoin/BRTransaction.h
    BRTransaction BRTransactionParse(byte[] buf, SizeT bufLen);
    BRTransaction BRTransactionCopy(BRTransaction tx);
    SizeT BRTransactionSerialize(BRTransaction tx, byte[] buf, SizeT bufLen);
    void BRTransactionFree(BRTransaction tx);

    // bitcoin/BRWallet.h
    BRAddress.ByValue BRWalletLegacyAddress(BRWallet wallet);
    int BRWalletContainsAddress(BRWallet wallet, String addr);
    long BRWalletBalance(BRWallet wallet);
    long BRWalletFeePerKb(BRWallet wallet);
    void BRWalletSetFeePerKb(BRWallet wallet, long feePerKb);
    BRTransaction BRWalletCreateTransaction(BRWallet wallet, long amount, String addr);
    long BRWalletAmountReceivedFromTx(BRWallet wallet, BRTransaction tx);
    long BRWalletAmountSentByTx(BRWallet wallet, BRTransaction tx);
    long BRWalletFeeForTx(BRWallet wallet, BRTransaction tx);
    long BRWalletFeeForTxAmount(BRWallet wallet, long amount);

    // bitcoin/BRWalletManager.h
    int bwmAnnounceBlockNumber(BRWalletManager manager, int rid, long blockNumber);
    int bwmAnnounceTransaction(BRWalletManager manager, int id, BRTransaction transaction);
    void bwmAnnounceTransactionComplete(BRWalletManager manager, int id, int success);
    void bwmAnnounceSubmit(BRWalletManager manager, int rid, BRTransaction transaction, int error);
    BRWalletManager BRWalletManagerNew(BRWalletManagerClient.ByValue client, BRMasterPubKey.ByValue mpk, BRChainParams params, int earliestKeyTime, int mode, String storagePath);
    void BRWalletManagerFree(BRWalletManager manager);
    void BRWalletManagerConnect(BRWalletManager manager);
    void BRWalletManagerDisconnect(BRWalletManager manager);
    void BRWalletManagerScan(BRWalletManager manager);
    BRWallet BRWalletManagerGetWallet(BRWalletManager manager);
    void BRWalletManagerGenerateUnusedAddrs(BRWalletManager manager, int limit);
    BRAddress BRWalletManagerGetAllAddrs(BRWalletManager manager, SizeTByReference addressesCount);
    BRAddress BRWalletManagerGetAllAddrsLegacy(BRWalletManager manager, SizeTByReference addressesCount);
    void BRWalletManagerSubmitTransaction(BRWalletManager manager, BRTransaction transaction, byte[] seed, SizeT seedLen);

    // crypto/BRCryptoAccount.h
    UInt512.ByValue cryptoAccountDeriveSeed(String phrase);
    BRCryptoAccount cryptoAccountCreate(String paperKey);
    BRCryptoAccount cryptoAccountCreateFromSeedBytes(byte[] bytes);
    long cryptoAccountGetTimestamp(BRCryptoAccount account);
    void cryptoAccountSetTimestamp(BRCryptoAccount account, long timestamp);
    void cryptoAccountGive(BRCryptoAccount obj);

    // crypto/BRCryptoAddress.h
    Pointer cryptoAddressAsString(BRCryptoAddress address);
    int cryptoAddressIsIdentical(BRCryptoAddress a1, BRCryptoAddress a2);
    void cryptoAddressGive(BRCryptoAddress obj);

    // crypto/BRCryptoAmount.h
    BRCryptoAmount cryptoAmountCreateDouble(double value, BRCryptoUnit unit);
    BRCryptoAmount cryptoAmountCreateInteger(long value, BRCryptoUnit unit);
    BRCryptoAmount cryptoAmountCreateString(String value, int isNegative, BRCryptoUnit unit);
    BRCryptoCurrency cryptoAmountGetCurrency(BRCryptoAmount amount);
    int cryptoAmountIsNegative(BRCryptoAmount amount);
    int cryptoAmountIsCompatible(BRCryptoAmount a1, BRCryptoAmount a2);
    int cryptoAmountCompare(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountAdd(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountSub(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountNegate(BRCryptoAmount amount);
    double cryptoAmountGetDouble(BRCryptoAmount amount, BRCryptoUnit unit, IntByReference overflow);
    long cryptoAmountGetIntegerRaw(BRCryptoAmount amount, IntByReference overflow);
    UInt256.ByValue cryptoAmountGetValue(BRCryptoAmount amount);
    void cryptoAmountGive(BRCryptoAmount obj);

    // crypto/BRCryptoCurrency.h
    String cryptoCurrencyGetName(BRCryptoCurrency currency);
    String cryptoCurrencyGetCode(BRCryptoCurrency currency);
    String cryptoCurrencyGetType(BRCryptoCurrency currency);
    void cryptoCurrencyGive(BRCryptoCurrency obj);

    // crypto/BRCryptoPrivate.h
    BRMasterPubKey.ByValue cryptoAccountAsBTC(BRCryptoAccount account);
    BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress.ByValue btc);
    BRCryptoAddress cryptoAddressCreateAsETH(BREthereumAddress.ByValue address);
    BRCryptoAmount cryptoAmountCreate (BRCryptoCurrency currency, int isNegative, UInt256.ByValue value);
    BRCryptoCurrency cryptoCurrencyCreate(String name, String code, String type);
    BRCryptoUnit cryptoUnitCreateAsBase(BRCryptoCurrency currency, String name, String symbol);
    BRCryptoUnit cryptoUnitCreate(BRCryptoCurrency currency, String name, String symbol, BRCryptoUnit base, byte decimals);

    // crypto/BRCryptoUnit.h
    String cryptoUnitGetName(BRCryptoUnit unit);
    String cryptoUnitGetSymbol(BRCryptoUnit unit);
    BRCryptoCurrency cryptoUnitGetCurrency(BRCryptoUnit unit);
    byte cryptoUnitGetBaseDecimalOffset(BRCryptoUnit unit);
    int cryptoUnitIsCompatible(BRCryptoUnit u1, BRCryptoUnit u2);
    void cryptoUnitGive(BRCryptoUnit obj);

    // ethereum/base/BREthereumAddress.h
    BREthereumAddress.ByValue addressCreate(String address);
    int addressValidateString(String addr);

    // ethereum/util/BRUtilMath.h
    UInt256.ByValue createUInt256(long value);
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    // support/BRAddress.h
    int BRAddressIsValid(String addr);
}
