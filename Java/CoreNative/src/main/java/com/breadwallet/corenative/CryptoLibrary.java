/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
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
import com.breadwallet.corenative.crypto.BRCryptoCWMClientCallbackState;
import com.breadwallet.corenative.crypto.BRCryptoCipher;
import com.breadwallet.corenative.crypto.BRCryptoCoder;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.BRCryptoHash;
import com.breadwallet.corenative.crypto.BRCryptoHasher;
import com.breadwallet.corenative.crypto.BRCryptoKey;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoPeer;
import com.breadwallet.corenative.crypto.BRCryptoSigner;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferState;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerState;
import com.breadwallet.corenative.crypto.BRCryptoWalletMigrator;
import com.breadwallet.corenative.crypto.BRCryptoWalletMigratorStatus;
import com.breadwallet.corenative.crypto.BRCryptoWalletSweeper;
import com.breadwallet.corenative.support.BRDisconnectReason;
import com.breadwallet.corenative.support.BRSyncStoppedReason;
import com.breadwallet.corenative.support.BRTransferSubmitError;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;

import java.nio.ByteBuffer;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

    // crypto/BRCryptoNetwork.h
    Pointer cryptoNetworkGetUids(BRCryptoNetwork network);
    Pointer cryptoNetworkGetName(BRCryptoNetwork network);
    int cryptoNetworkIsMainnet(BRCryptoNetwork network);
    BRCryptoCurrency cryptoNetworkGetCurrency(BRCryptoNetwork network);
    BRCryptoUnit cryptoNetworkGetUnitAsDefault(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoUnit cryptoNetworkGetUnitAsBase(BRCryptoNetwork network, BRCryptoCurrency currency);
    long cryptoNetworkGetHeight(BRCryptoNetwork network);
    int cryptoNetworkGetConfirmationsUntilFinal(BRCryptoNetwork network);
    void cryptoNetworkSetConfirmationsUntilFinal(BRCryptoNetwork network, int confirmationsUntilFinal);
    SizeT cryptoNetworkGetCurrencyCount(BRCryptoNetwork network);
    BRCryptoCurrency cryptoNetworkGetCurrencyAt(BRCryptoNetwork network, SizeT index);
    int cryptoNetworkHasCurrency(BRCryptoNetwork network, BRCryptoCurrency currency);
    SizeT cryptoNetworkGetUnitCount(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoUnit cryptoNetworkGetUnitAt(BRCryptoNetwork network, BRCryptoCurrency currency, SizeT index);
    void cryptoNetworkSetNetworkFees(BRCryptoNetwork network, BRCryptoNetworkFee[] fees, SizeT count);
    Pointer cryptoNetworkGetNetworkFees(BRCryptoNetwork network, SizeTByReference count);
    BRCryptoAddress cryptoNetworkCreateAddressFromString(BRCryptoNetwork network, String address);
    BRCryptoNetwork cryptoNetworkTake(BRCryptoNetwork obj);
    void cryptoNetworkGive(BRCryptoNetwork obj);

    long cryptoNetworkFeeGetConfirmationTimeInMilliseconds(BRCryptoNetworkFee fee);
    int cryptoNetworkFeeEqual(BRCryptoNetworkFee nf1, BRCryptoNetworkFee nf2);
    void cryptoNetworkFeeGive(BRCryptoNetworkFee obj);

    // crypto/BRCryptoPrivate.h
    void cryptoNetworkSetHeight(BRCryptoNetwork network, long height);
    void cryptoNetworkSetCurrency(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoNetwork cryptoNetworkCreateAsBTC(String uids, String name, byte forkId, Pointer params);
    BRCryptoNetwork cryptoNetworkCreateAsETH(String uids, String name, int chainId, Pointer net);
    BRCryptoNetwork cryptoNetworkCreateAsGEN(String uids, String name, byte isMainnet);
    void cryptoNetworkAddCurrency(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit baseUnit, BRCryptoUnit defaultUnit);
    void cryptoNetworkAddCurrencyUnit(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit unit);
    BRCryptoNetworkFee cryptoNetworkFeeCreate(long timeInternalInMilliseconds, BRCryptoAmount pricePerCostFactor, BRCryptoUnit pricePerCostFactorUnit);
    void cryptoNetworkAddNetworkFee(BRCryptoNetwork network, BRCryptoNetworkFee fee);

    // crypto/BRCryptoWalletManager.h
    BRCryptoWalletManager cryptoWalletManagerCreate(BRCryptoCWMListener.ByValue listener,
                                                    BRCryptoCWMClient.ByValue client,
                                                    BRCryptoAccount account,
                                                    BRCryptoNetwork network,
                                                    int mode,
                                                    int addressScheme,
                                                    String path);
    BRCryptoNetwork cryptoWalletManagerGetNetwork(BRCryptoWalletManager cwm);
    BRCryptoAccount cryptoWalletManagerGetAccount(BRCryptoWalletManager cwm);
    int cryptoWalletManagerGetMode(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetMode(BRCryptoWalletManager cwm, int mode);
    BRCryptoWalletManagerState.ByValue cryptoWalletManagerGetState(BRCryptoWalletManager cwm);
    int cryptoWalletManagerGetAddressScheme (BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetAddressScheme (BRCryptoWalletManager cwm, int scheme);
    Pointer cryptoWalletManagerGetPath(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetNetworkReachable(BRCryptoWalletManager manager, int isNetworkReachable);
    BRCryptoWallet cryptoWalletManagerGetWallet(BRCryptoWalletManager cwm);
    Pointer cryptoWalletManagerGetWallets(BRCryptoWalletManager cwm, SizeTByReference count);
    int cryptoWalletManagerHasWallet(BRCryptoWalletManager cwm, BRCryptoWallet wallet);
    BRCryptoWallet cryptoWalletManagerRegisterWallet(BRCryptoWalletManager cwm, BRCryptoCurrency currency);
    void cryptoWalletManagerConnect(BRCryptoWalletManager cwm, BRCryptoPeer peer);
    void cryptoWalletManagerDisconnect(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSync(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSyncToDepth(BRCryptoWalletManager cwm, int depth);
    void cryptoWalletManagerStop(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSubmit(BRCryptoWalletManager cwm, BRCryptoWallet wid, BRCryptoTransfer tid, ByteBuffer paperKey);
    void cryptoWalletManagerSubmitForKey(BRCryptoWalletManager cwm, BRCryptoWallet wid, BRCryptoTransfer tid, BRCryptoKey key);
    void cwmAnnounceGetBlockNumberSuccessAsInteger(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState,long blockNumber);
    void cwmAnnounceGetBlockNumberSuccessAsString(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String blockNumber);
    void cwmAnnounceGetBlockNumberFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceGetTransactionsItemBTC(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState,
                                           byte[] transaction, SizeT transactionLength, long timestamp, long blockHeight);
    void cwmAnnounceGetTransactionsItemETH(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState,
                                           String hash, String sourceAddr, String targetAddr, String contractAddr,
                                           String amount, String gasLimit, String gasPrice, String data, String nonce,
                                           String gasUsed, String blockNumber, String blockHash,
                                           String blockConfirmations, String blockTransacionIndex, String blockTimestamp,
                                           String isError);
    void cwmAnnounceGetTransactionsItemGEN(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState,
                                           byte[] transaction, SizeT transactionLength, long timestamp, long blockHeight);
    void cwmAnnounceGetTransactionsComplete(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, int success);
    void cwmAnnounceSubmitTransferSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceSubmitTransferSuccessForHash(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String hash);
    void cwmAnnounceSubmitTransferFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceGetBalanceSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String balance);
    void cwmAnnounceGetBalanceFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceGetBlocksSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, int length,long[] blockArray);
    void cwmAnnounceGetBlocksFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceGetGasPriceSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String gasPrice);
    void cwmAnnounceGetGasPriceFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    void cwmAnnounceGetGasEstimateSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice);
    void cwmAnnounceGetGasEstimateFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, int status);
    void cwmAnnounceGetLogsItem(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String hash,
                                String contract, int size, StringArray topicsArray, String data, String gasPrice,
                                String gasUsed, String logIndex, String blockNumber, String blockTransactionIndex,
                                String blockTimestamp);
    void cwmAnnounceGetLogsComplete(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, int success);
    void cwmAnnounceGetTokensItem(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String address,
                                  String symbol, String name, String description, int intValue, String gasLimit,
                                  String gasPrice);
    void cwmAnnounceGetTokensComplete(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, int success);
    void cwmAnnounceGetNonceSuccess(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState, String address,String nonce);
    void cwmAnnounceGetNonceFailure(BRCryptoWalletManager cwm, BRCryptoCWMClientCallbackState callbackState);
    BRCryptoWalletManager cryptoWalletManagerTake(BRCryptoWalletManager obj);
    void cryptoWalletManagerGive(BRCryptoWalletManager obj);

    // crypto/BRCryptoWalletManager.h
    int cryptoWalletSweeperValidateSupported(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoKey key, BRCryptoWallet wallet);
    BRCryptoWalletSweeper cryptoWalletSweeperCreateAsBtc(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoKey key, int scheme);
    BRCryptoKey cryptoWalletSweeperGetKey(BRCryptoWalletSweeper sweeper);
    BRCryptoAmount cryptoWalletSweeperGetBalance(BRCryptoWalletSweeper sweeper);
    Pointer cryptoWalletSweeperGetAddress(BRCryptoWalletSweeper sweeper);
    int cryptoWalletSweeperHandleTransactionAsBTC(BRCryptoWalletSweeper sweeper, byte[] transaction, SizeT transactionLen);
    int cryptoWalletSweeperValidate(BRCryptoWalletSweeper sweeper);
    void cryptoWalletSweeperRelease(BRCryptoWalletSweeper sweeper);

    // crypto/BRCryptoWalletManager.h
    BRCryptoWalletMigrator cryptoWalletMigratorCreate(BRCryptoNetwork network, String storagePath);
    BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandleTransactionAsBTC (BRCryptoWalletMigrator migrator, byte[] bytes, SizeT bytesCount, int blockHeight, int timestamp);
    BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandleBlockBytesAsBTC(BRCryptoWalletMigrator migrator, byte[] buffer, SizeT bufferLen, int height);
    BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandlePeerAsBTC (BRCryptoWalletMigrator migrator, int address, short port, long services, int timestamp);
    void cryptoWalletMigratorRelease(BRCryptoWalletMigrator migrator);

    // ethereum/util/BRUtilMath.h
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    // support/BRSyncMode.h
    Pointer BRSyncStoppedReasonGetMessage(BRSyncStoppedReason reason);
    Pointer BRDisconnectReasonGetMessage(BRDisconnectReason reason);
    Pointer BRTransferSubmitErrorGetMessage(BRTransferSubmitError error);
}
