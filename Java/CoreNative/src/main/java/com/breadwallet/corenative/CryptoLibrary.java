/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoAccount;
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoCallbackHandle;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.BRCryptoHash;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferState;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.ethereum.BREthereumEwm;
import com.breadwallet.corenative.ethereum.BREthereumNetwork;
import com.breadwallet.corenative.ethereum.BREthereumToken;
import com.breadwallet.corenative.ethereum.BREthereumTransfer;
import com.breadwallet.corenative.ethereum.BREthereumWallet;
import com.breadwallet.corenative.support.BRAddress;
import com.breadwallet.corenative.ethereum.BREthereumAddress;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.support.UInt512;
import com.breadwallet.corenative.utility.SizeT;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

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
    BRCryptoAddress cryptoAddressTake(BRCryptoAddress obj);
    void cryptoAddressGive(BRCryptoAddress obj);

    // crypto/BRCryptoAmount.h
    BRCryptoAmount cryptoAmountCreateDouble(double value, BRCryptoUnit unit);
    BRCryptoAmount cryptoAmountCreateInteger(long value, BRCryptoUnit unit);
    BRCryptoAmount cryptoAmountCreateString(String value, int isNegative, BRCryptoUnit unit);
    BRCryptoCurrency cryptoAmountGetCurrency(BRCryptoAmount amount);
    int cryptoAmountHasCurrency(BRCryptoAmount amount, BRCryptoCurrency currency);
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
    Pointer cryptoCurrencyGetUids(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetName(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetCode(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetType(BRCryptoCurrency currency);
    int cryptoCurrencyIsIdentical(BRCryptoCurrency c1, BRCryptoCurrency c2);
    void cryptoCurrencyGive(BRCryptoCurrency obj);

    // crypto/BRCryptoFeeBasis.h
    int cryptoFeeBasisGetType(BRCryptoFeeBasis feeBasis);
    BRCryptoFeeBasis cryptoFeeBasisTake(BRCryptoFeeBasis obj);
    void cryptoFeeBasisGive(BRCryptoFeeBasis obj);

    // crypto/BRCryptoHash.h
    int cryptoHashEqual(BRCryptoHash h1, BRCryptoHash h2);
    Pointer cryptoHashString(BRCryptoHash hash);
    int cryptoHashGetHashValue(BRCryptoHash hash);
    BRCryptoHash cryptoHashTake(BRCryptoHash obj);
    void cryptoHashGive(BRCryptoHash obj);

    // crypto/BRCryptoNetwork.h
    int cryptoNetworkGetType(BRCryptoNetwork network);
    Pointer cryptoNetworkGetUids(BRCryptoNetwork network);
    Pointer cryptoNetworkGetName(BRCryptoNetwork network);
    int cryptoNetworkIsMainnet(BRCryptoNetwork network);
    BRCryptoCurrency cryptoNetworkGetCurrency(BRCryptoNetwork network);
    BRCryptoUnit cryptoNetworkGetUnitAsDefault(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoUnit cryptoNetworkGetUnitAsBase(BRCryptoNetwork network, BRCryptoCurrency currency);
    long cryptoNetworkGetHeight(BRCryptoNetwork network);
    SizeT cryptoNetworkGetCurrencyCount(BRCryptoNetwork network);
    BRCryptoCurrency cryptoNetworkGetCurrencyAt(BRCryptoNetwork network, SizeT index);
    int cryptoNetworkHasCurrency(BRCryptoNetwork network, BRCryptoCurrency currency);
    SizeT cryptoNetworkGetUnitCount(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoUnit cryptoNetworkGetUnitAt(BRCryptoNetwork network, BRCryptoCurrency currency, SizeT index);
    BRCryptoNetwork cryptoNetworkTake(BRCryptoNetwork obj);
    void cryptoNetworkGive(BRCryptoNetwork obj);

    // crypto/BRCryptoPrivate.h
    BRCryptoAddress cryptoAddressCreateAsBTC(BRAddress.ByValue btc);
    BRCryptoAddress cryptoAddressCreateAsETH(BREthereumAddress.ByValue address);
    BRCryptoCurrency cryptoCurrencyCreate(String uids, String name, String code, String type);
    void cryptoNetworkSetHeight(BRCryptoNetwork network, long height);
    void cryptoNetworkSetCurrency(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoNetwork cryptoNetworkCreateAsBTC(String uids, String name, byte forkId, Pointer params);
    BRCryptoNetwork cryptoNetworkCreateAsETH(String uids, String name, int chainId, Pointer net);
    BRCryptoNetwork cryptoNetworkCreateAsGEN(String uids, String name);
    void cryptoNetworkAddCurrency(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit baseUnit, BRCryptoUnit defaultUnit);
    void cryptoNetworkAddCurrencyUnit(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit unit);
    BRCryptoUnit cryptoUnitCreateAsBase(BRCryptoCurrency currency, String uids, String name, String symbol);
    BRCryptoUnit cryptoUnitCreate(BRCryptoCurrency currency, String uids, String name, String symbol, BRCryptoUnit base, byte decimals);

    // crypto/BRCryptoTransfer.h
    BRCryptoAddress cryptoTransferGetSourceAddress(BRCryptoTransfer transfer);
    BRCryptoAddress cryptoTransferGetTargetAddress(BRCryptoTransfer transfer);
    BRCryptoAmount cryptoTransferGetAmount(BRCryptoTransfer transfer);
    BRCryptoAmount cryptoTransferGetAmountDirected(BRCryptoTransfer transfer);
    BRCryptoAmount cryptoTransferGetFee(BRCryptoTransfer transfer);
    int cryptoTransferGetDirection(BRCryptoTransfer transfer);
    BRCryptoTransferState.ByValue cryptoTransferGetState(BRCryptoTransfer transfer);
    BRCryptoHash cryptoTransferGetHash(BRCryptoTransfer transfer);
    BRCryptoFeeBasis cryptoTransferGetFeeBasis(BRCryptoTransfer transfer);
    int cryptoTransferEqual(BRCryptoTransfer transfer1, BRCryptoTransfer transfer2);
    BRCryptoTransfer cryptoTransferTake(BRCryptoTransfer obj);
    void cryptoTransferGive(BRCryptoTransfer obj);

    // crypto/BRCryptoUnit.h
    Pointer cryptoUnitGetUids(BRCryptoUnit unit);
    Pointer cryptoUnitGetName(BRCryptoUnit unit);
    Pointer cryptoUnitGetSymbol(BRCryptoUnit unit);
    BRCryptoCurrency cryptoUnitGetCurrency(BRCryptoUnit unit);
    int cryptoUnitHasCurrency(BRCryptoUnit unit, BRCryptoCurrency currency);
    BRCryptoUnit cryptoUnitGetBaseUnit(BRCryptoUnit unit);
    byte cryptoUnitGetBaseDecimalOffset(BRCryptoUnit unit);
    int cryptoUnitIsCompatible(BRCryptoUnit u1, BRCryptoUnit u2);
    int cryptoUnitIsIdentical(BRCryptoUnit u1, BRCryptoUnit u2);
    BRCryptoUnit cryptoUnitTake(BRCryptoUnit obj);
    void cryptoUnitGive(BRCryptoUnit obj);

    // crypto/BRCryptoWallet.h
    int cryptoWalletGetState(BRCryptoWallet wallet);
    void cryptoWalletSetState(BRCryptoWallet wallet, int state);
    BRCryptoAmount cryptoWalletGetBalance(BRCryptoWallet wallet);
    SizeT cryptoWalletGetTransferCount(BRCryptoWallet wallet);
    BRCryptoTransfer cryptoWalletGetTransfer(BRCryptoWallet wallet, SizeT index);
    int cryptoWalletHasTransfer(BRCryptoWallet wallet, BRCryptoTransfer transfer);
    BRCryptoAddress cryptoWalletGetAddress(BRCryptoWallet wallet);
    BRCryptoFeeBasis cryptoWalletGetDefaultFeeBasis(BRCryptoWallet wallet);
    void cryptoWalletSetDefaultFeeBasis(BRCryptoWallet wallet, BRCryptoFeeBasis feeBasis);
    BRCryptoTransfer cryptoWalletCreateTransfer(BRCryptoWallet wallet, BRCryptoAddress target, BRCryptoAmount amount, BRCryptoFeeBasis feeBasis);
    BRCryptoAmount cryptoWalletEstimateFee(BRCryptoWallet wallet, BRCryptoAmount amount, BRCryptoFeeBasis feeBasis, BRCryptoUnit feeUnit);
    int cryptoWalletEqual(BRCryptoWallet w1, BRCryptoWallet w2);
    BRCryptoWallet cryptoWalletTake(BRCryptoWallet obj);
    void cryptoWalletGive(BRCryptoWallet obj);

    // crypto/BRCryptoWalletManager.h
    BRCryptoWalletManager cryptoWalletManagerCreate(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client, BRCryptoAccount account, BRCryptoNetwork network, int mode, String path);
    BRCryptoNetwork cryptoWalletManagerGetNetwork(BRCryptoWalletManager cwm);
    BRCryptoAccount cryptoWalletManagerGetAccount(BRCryptoWalletManager cwm);
    int cryptoWalletManagerGetMode(BRCryptoWalletManager cwm);
    BRCryptoUnit cryptoWalletGetUnit(BRCryptoWallet wallet);
    BRCryptoUnit cryptoWalletGetUnitForFee(BRCryptoWallet wallet);
    BRCryptoCurrency cryptoWalletGetCurrency(BRCryptoWallet wallet);
    int cryptoWalletManagerGetState(BRCryptoWalletManager cwm);
    Pointer cryptoWalletManagerGetPath(BRCryptoWalletManager cwm);
    BRCryptoWallet cryptoWalletManagerGetWallet(BRCryptoWalletManager cwm);
    SizeT cryptoWalletManagerGetWalletsCount(BRCryptoWalletManager cwm);
    BRCryptoWallet cryptoWalletManagerGetWalletAtIndex(BRCryptoWalletManager cwm, SizeT index);
    int cryptoWalletManagerHasWallet(BRCryptoWalletManager manager, BRCryptoWallet wallet);
    void cryptoWalletManagerConnect(BRCryptoWalletManager cwm);
    void cryptoWalletManagerDisconnect(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSync(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSubmit(BRCryptoWalletManager cwm, BRCryptoWallet wid, BRCryptoTransfer tid, String paperKey);
    void cwmAnnounceBlockNumber(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, long blockHeight, int success);
    void cwmAnnounceTransaction(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, byte[] transaction, SizeT transactionLength, long timestamp, long blockHeight);
    void cwmAnnounceTransactionComplete(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, int success);
    void cwmAnnounceSubmit(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, int success);
    void cwmAnnounceBalance(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, long balance, int success);
    void cwmAnnounceGasPrice(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, long gasPrice, int success);
    void cwmAnnounceGasEstimate(BRCryptoWalletManager manager, BRCryptoCallbackHandle handle, long gasEstimate, int success);
    BRCryptoWalletManager cryptoWalletManagerTake(BRCryptoWalletManager obj);
    void cryptoWalletManagerGive(BRCryptoWalletManager obj);

    // ethereum/base/BREthereumAddress.h
    BREthereumAddress.ByValue addressCreate(String address);
    int addressValidateString(String addr);

    // ethereum/blockchain/BREthereumNetwork.h
    Pointer networkGetName(BREthereumNetwork network);

    // ethereum/contract/BREthereumToken.h
    Pointer tokenGetAddress(BREthereumToken token);

    // ethereum/ewm/BREthereumClient.h
    int ewmAnnounceWalletBalance(BREthereumEwm ewm, BREthereumWallet wid, String data, int rid);
    int ewmAnnounceGasPrice(BREthereumEwm ewm, BREthereumWallet wid, String data, int rid);
    int ewmAnnounceGasEstimate(BREthereumEwm ewm, BREthereumWallet wid, BREthereumTransfer tid, String gasEstimate, int rid);
    int ewmAnnounceSubmitTransfer(BREthereumEwm ewm, BREthereumWallet wid, BREthereumTransfer tid, String hash,
                                  int errorCode, String errorMessage, int rid);
    int ewmAnnounceTransaction(BREthereumEwm ewm, int rid, String hash, String sourceAddr, String targetAddr,
                               String contractAddr, String amount, String gasLimit, String gasPrice, String data,
                               String nonce, String gasUsed, String blockNumber, String blockHash,
                               String blockConfirmations, String blockTransacionIndex, String blockTimestamp,
                               String isError);
    int ewmAnnounceTransactionComplete(BREthereumEwm ewm, int rid, int success);
    int ewmAnnounceLog(BREthereumEwm ewm, int rid, String hash, String contract, int topicCount, StringArray topics,
                       String data, String gasPrice, String gasUsed, String logIndex, String blockNumber,
                       String blockTransactionIndex, String blockTimestamp);
    int ewmAnnounceLogComplete(BREthereumEwm ewm, int rid, int success);
    int ewmAnnounceToken(BREthereumEwm ewm, int rid, String address, String symbol, String name, String description,
                         int decimals, String defaultGasLimit, String defaultGasPrice);
    int ewmAnnounceTokenComplete(BREthereumEwm ewm, int rid, int success);
    int ewmAnnounceBlockNumber(BREthereumEwm bwm, String blockNumber, int rid);
    int ewmAnnounceNonce(BREthereumEwm ewm, String address, String nonce, int rid);
    int ewmAnnounceBlocks(BREthereumEwm ewm, int rid, int blockNumbersCount, long[] blockNumbers);

    // ethereum/ewm/BREthereumEWM.h
    BREthereumToken ewmWalletGetToken(BREthereumEwm ewm, BREthereumWallet wid);
    BREthereumNetwork ewmGetNetwork(BREthereumEwm ewm);

    // ethereum/util/BRUtilMath.h
    UInt256.ByValue createUInt256(long value);
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    // support/BRAddress.h
    int BRAddressIsValid(String addr);
}
