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

    // crypto/BRCryptoAccount.h
    BRCryptoAccount cryptoAccountCreate(ByteBuffer phrase, long timestamp);
    BRCryptoAccount cryptoAccountCreateFromSerialization(byte[] serialization, SizeT serializationLength);
    long cryptoAccountGetTimestamp(BRCryptoAccount account);
    Pointer cryptoAccountGetFileSystemIdentifier(BRCryptoAccount account);
    Pointer cryptoAccountSerialize(BRCryptoAccount account, SizeTByReference count);
    int cryptoAccountValidateSerialization(BRCryptoAccount account, byte[] serialization, SizeT count);
    int cryptoAccountValidateWordsList(SizeT count);
    Pointer cryptoAccountGeneratePaperKey(StringArray words);
    int cryptoAccountValidatePaperKey(ByteBuffer phraseBuffer, StringArray wordsArray);
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
    BRCryptoUnit cryptoAmountGetUnit(BRCryptoAmount brCryptoAmount);
    int cryptoAmountHasCurrency(BRCryptoAmount amount, BRCryptoCurrency currency);
    int cryptoAmountIsNegative(BRCryptoAmount amount);
    int cryptoAmountIsCompatible(BRCryptoAmount a1, BRCryptoAmount a2);
    int cryptoAmountCompare(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountAdd(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountSub(BRCryptoAmount a1, BRCryptoAmount a2);
    BRCryptoAmount cryptoAmountNegate(BRCryptoAmount amount);
    BRCryptoAmount cryptoAmountConvertToUnit(BRCryptoAmount amount, BRCryptoUnit unit);
    double cryptoAmountGetDouble(BRCryptoAmount amount, BRCryptoUnit unit, IntByReference overflow);
    UInt256.ByValue cryptoAmountGetValue(BRCryptoAmount amount);
    void cryptoAmountGive(BRCryptoAmount obj);

    // crypto/BRCryptoCurrency.h
    Pointer cryptoCurrencyGetUids(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetName(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetCode(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetType(BRCryptoCurrency currency);
    Pointer cryptoCurrencyGetIssuer(BRCryptoCurrency currency);
    int cryptoCurrencyIsIdentical(BRCryptoCurrency c1, BRCryptoCurrency c2);
    void cryptoCurrencyGive(BRCryptoCurrency obj);

    // crypto/BRCryptoFeeBasis.h
    BRCryptoAmount cryptoFeeBasisGetPricePerCostFactor (BRCryptoFeeBasis feeBasis);
    BRCryptoUnit cryptoFeeBasisGetPricePerCostFactorUnit (BRCryptoFeeBasis feeBasis);
    double cryptoFeeBasisGetCostFactor (BRCryptoFeeBasis feeBasis);
    BRCryptoAmount cryptoFeeBasisGetFee (BRCryptoFeeBasis feeBasis);
    int cryptoFeeBasisIsIdentical(BRCryptoFeeBasis f1, BRCryptoFeeBasis f2);
    void cryptoFeeBasisGive(BRCryptoFeeBasis obj);

    // crypto/BRCryptoKey.h
    int cryptoKeyIsProtectedPrivate(ByteBuffer keyBuffer);
    BRCryptoKey cryptoKeyCreateFromPhraseWithWords(ByteBuffer phraseBuffer, StringArray wordsArray);
    BRCryptoKey cryptoKeyCreateFromStringPrivate(ByteBuffer stringBuffer);
    BRCryptoKey cryptoKeyCreateFromStringProtectedPrivate(ByteBuffer stringBuffer, ByteBuffer phraseBuffer);
    BRCryptoKey cryptoKeyCreateFromStringPublic(ByteBuffer stringBuffer);
    BRCryptoKey cryptoKeyCreateForPigeon(BRCryptoKey key, byte[] nonce, SizeT nonceCount);
    BRCryptoKey cryptoKeyCreateForBIP32ApiAuth(ByteBuffer phraseBuffer, StringArray wordsArray);
    BRCryptoKey cryptoKeyCreateForBIP32BitID(ByteBuffer phraseBuffer, int index, String uri, StringArray wordsArray);
    BRCryptoKey cryptoKeyCreateFromSecret(UInt256.ByValue secret);
    void cryptoKeyProvidePublicKey(BRCryptoKey key, int useCompressed, int compressed);
    int cryptoKeyHasSecret(BRCryptoKey key);
    int cryptoKeyPublicMatch(BRCryptoKey key, BRCryptoKey other);
    int cryptoKeySecretMatch(BRCryptoKey key, BRCryptoKey other);
    Pointer cryptoKeyEncodePrivate(BRCryptoKey key);
    Pointer cryptoKeyEncodePublic(BRCryptoKey key);
    UInt256.ByValue cryptoKeyGetSecret(BRCryptoKey key);
    void cryptoKeyGive(BRCryptoKey key);

    // crypto/BRCryptoHash.h
    int cryptoHashEqual(BRCryptoHash h1, BRCryptoHash h2);
    Pointer cryptoHashString(BRCryptoHash hash);
    int cryptoHashGetHashValue(BRCryptoHash hash);
    void cryptoHashGive(BRCryptoHash obj);

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
    void cryptoNetworkGive(BRCryptoNetwork obj);

    BRCryptoPeer cryptoPeerCreate(BRCryptoNetwork network, String address, short port, String publicKey);
    BRCryptoNetwork.OwnedBRCryptoNetwork cryptoPeerGetNetwork(BRCryptoPeer peer);
    Pointer cryptoPeerGetAddress(BRCryptoPeer peer);
    Pointer cryptoPeerGetPublicKey(BRCryptoPeer peer);
    short cryptoPeerGetPort(BRCryptoPeer peer);
    int cryptoPeerIsIdentical(BRCryptoPeer p1, BRCryptoPeer p2);
    void cryptoPeerGive(BRCryptoPeer peer);

    long cryptoNetworkFeeGetConfirmationTimeInMilliseconds(BRCryptoNetworkFee fee);
    int cryptoNetworkFeeEqual(BRCryptoNetworkFee nf1, BRCryptoNetworkFee nf2);
    void cryptoNetworkFeeGive(BRCryptoNetworkFee obj);

    // crypto/BRCryptoPrivate.h
    BRCryptoCurrency cryptoCurrencyCreate(String uids, String name, String code, String type, String issuer);
    void cryptoNetworkSetHeight(BRCryptoNetwork network, long height);
    void cryptoNetworkSetCurrency(BRCryptoNetwork network, BRCryptoCurrency currency);
    BRCryptoNetwork.OwnedBRCryptoNetwork cryptoNetworkCreateAsBTC(String uids, String name, byte forkId, Pointer params);
    BRCryptoNetwork.OwnedBRCryptoNetwork cryptoNetworkCreateAsETH(String uids, String name, int chainId, Pointer net);
    BRCryptoNetwork.OwnedBRCryptoNetwork cryptoNetworkCreateAsGEN(String uids, String name, byte isMainnet);
    void cryptoNetworkAddCurrency(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit baseUnit, BRCryptoUnit defaultUnit);
    void cryptoNetworkAddCurrencyUnit(BRCryptoNetwork network, BRCryptoCurrency currency, BRCryptoUnit unit);
    BRCryptoUnit cryptoUnitCreateAsBase(BRCryptoCurrency currency, String uids, String name, String symbol);
    BRCryptoUnit cryptoUnitCreate(BRCryptoCurrency currency, String uids, String name, String symbol, BRCryptoUnit base, byte decimals);
    BRCryptoNetworkFee cryptoNetworkFeeCreate(long timeInternalInMilliseconds, BRCryptoAmount pricePerCostFactor, BRCryptoUnit pricePerCostFactorUnit);
    void cryptoNetworkAddNetworkFee(BRCryptoNetwork network, BRCryptoNetworkFee fee);

    // crypto/BRCryptoTransfer.h
    BRCryptoAddress cryptoTransferGetSourceAddress(BRCryptoTransfer transfer);
    BRCryptoAddress cryptoTransferGetTargetAddress(BRCryptoTransfer transfer);
    BRCryptoAmount cryptoTransferGetAmount(BRCryptoTransfer transfer);
    BRCryptoAmount cryptoTransferGetAmountDirected(BRCryptoTransfer transfer);
    int cryptoTransferGetDirection(BRCryptoTransfer transfer);
    BRCryptoTransferState.ByValue cryptoTransferGetState(BRCryptoTransfer transfer);
    BRCryptoHash cryptoTransferGetHash(BRCryptoTransfer transfer);
    BRCryptoUnit cryptoTransferGetUnitForAmount (BRCryptoTransfer transfer);
    BRCryptoUnit cryptoTransferGetUnitForFee (BRCryptoTransfer transfer);
    BRCryptoFeeBasis cryptoTransferGetEstimatedFeeBasis (BRCryptoTransfer transfer);
    BRCryptoFeeBasis cryptoTransferGetConfirmedFeeBasis (BRCryptoTransfer transfer);
    int cryptoTransferEqual(BRCryptoTransfer transfer1, BRCryptoTransfer transfer2);
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
    void cryptoUnitGive(BRCryptoUnit obj);

    // crypto/BRCryptoWallet.h
    int cryptoWalletGetState(BRCryptoWallet wallet);
    void cryptoWalletSetState(BRCryptoWallet wallet, int state);
    BRCryptoAmount cryptoWalletGetBalance(BRCryptoWallet wallet);
    Pointer cryptoWalletGetTransfers(BRCryptoWallet wallet, SizeTByReference count);
    int cryptoWalletHasTransfer(BRCryptoWallet wallet, BRCryptoTransfer transfer);
    BRCryptoAddress cryptoWalletGetAddress(BRCryptoWallet wallet, int addressScheme);
    BRCryptoFeeBasis cryptoWalletGetDefaultFeeBasis(BRCryptoWallet wallet);
    void cryptoWalletSetDefaultFeeBasis(BRCryptoWallet wallet, BRCryptoFeeBasis feeBasis);
    BRCryptoTransfer.OwnedBRCryptoTransfer cryptoWalletCreateTransfer(BRCryptoWallet wallet, BRCryptoAddress target, BRCryptoAmount amount, BRCryptoFeeBasis feeBasis);
    BRCryptoTransfer.OwnedBRCryptoTransfer cryptoWalletCreateTransferForWalletSweep(BRCryptoWallet wallet, BRCryptoWalletSweeper sweeper, BRCryptoFeeBasis feeBasis);
    void cryptoWalletEstimateFeeBasis(BRCryptoWallet wallet, Pointer cookie, BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee);
    void cryptoWalletEstimateFeeBasisForWalletSweep(BRCryptoWallet wallet, Pointer cookie, BRCryptoWalletSweeper sweeper, BRCryptoNetworkFee fee);
    void cryptoWalletGive(BRCryptoWallet obj);

    // crypto/BRCryptoWalletManager.h
    BRCryptoWalletManager.OwnedBRCryptoWalletManager cryptoWalletManagerCreate(BRCryptoCWMListener.ByValue listener,
                                                                               BRCryptoCWMClient.ByValue client,
                                                                               BRCryptoAccount account,
                                                                               BRCryptoNetwork network,
                                                                               int mode,
                                                                               int addressScheme,
                                                                               String path);
    BRCryptoNetwork.OwnedBRCryptoNetwork cryptoWalletManagerGetNetwork(BRCryptoWalletManager cwm);
    BRCryptoAccount cryptoWalletManagerGetAccount(BRCryptoWalletManager cwm);
    int cryptoWalletManagerGetMode(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetMode(BRCryptoWalletManager cwm, int mode);
    BRCryptoUnit cryptoWalletGetUnit(BRCryptoWallet wallet);
    BRCryptoUnit cryptoWalletGetUnitForFee(BRCryptoWallet wallet);
    BRCryptoCurrency cryptoWalletGetCurrency(BRCryptoWallet wallet);
    BRCryptoWalletManagerState.ByValue cryptoWalletManagerGetState(BRCryptoWalletManager cwm);
    int cryptoWalletManagerGetAddressScheme (BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetAddressScheme (BRCryptoWalletManager cwm, int scheme);
    Pointer cryptoWalletManagerGetPath(BRCryptoWalletManager cwm);
    void cryptoWalletManagerSetNetworkReachable(BRCryptoWalletManager manager, int isNetworkReachable);
    BRCryptoWallet.OwnedBRCryptoWallet cryptoWalletManagerGetWallet(BRCryptoWalletManager cwm);
    Pointer cryptoWalletManagerGetWallets(BRCryptoWalletManager cwm, SizeTByReference count);
    int cryptoWalletManagerHasWallet(BRCryptoWalletManager cwm, BRCryptoWallet wallet);
    BRCryptoWallet.OwnedBRCryptoWallet cryptoWalletManagerRegisterWallet(BRCryptoWalletManager cwm, BRCryptoCurrency currency);
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

    // crypto/BRCryptoCoder.h
    BRCryptoCoder.OwnedBRCryptoCoder cryptoCoderCreate(int type);
    SizeT cryptoCoderEncodeLength(BRCryptoCoder coder, byte[] src, SizeT srcLen);
    int cryptoCoderEncode(BRCryptoCoder coder, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    SizeT cryptoCoderDecodeLength(BRCryptoCoder coder, byte[] src);
    int cryptoCoderDecode(BRCryptoCoder coder, byte[] dst, SizeT dstLen, byte[] src);
    void cryptoCoderGive(BRCryptoCoder coder);

    // crypto/BRCryptoCipher.h
    BRCryptoCipher.OwnedBRCryptoCipher cryptoCipherCreateForAESECB(byte[] key, SizeT keyLen);
    BRCryptoCipher.OwnedBRCryptoCipher cryptoCipherCreateForChacha20Poly1305(BRCryptoKey key, byte[] nonce12, SizeT nonce12Len, byte[] ad, SizeT adLen);
    BRCryptoCipher.OwnedBRCryptoCipher cryptoCipherCreateForPigeon(BRCryptoKey privKey, BRCryptoKey pubKey, byte[] nonce12, SizeT nonce12Len);
    SizeT cryptoCipherEncryptLength(BRCryptoCipher cipher, byte[] src, SizeT srcLen);
    int cryptoCipherEncrypt(BRCryptoCipher cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    SizeT cryptoCipherDecryptLength(BRCryptoCipher cipher, byte[] src, SizeT srcLen);
    int cryptoCipherDecrypt(BRCryptoCipher cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    void cryptoCipherGive(BRCryptoCipher cipher);

    // crypto/BRCryptoHasher.h
    BRCryptoHasher.OwnedBRCryptoHasher cryptoHasherCreate(int type);
    SizeT cryptoHasherLength(BRCryptoHasher hasher);
    int cryptoHasherHash(BRCryptoHasher hasher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    void cryptoHasherGive(BRCryptoHasher hasher);

    // crypto/BRCryptoSigner.h
    BRCryptoSigner.OwnedBRCryptoSigner cryptoSignerCreate(int type);
    SizeT cryptoSignerSignLength(BRCryptoSigner signer, BRCryptoKey key, byte[] digest, SizeT digestlen);
    int cryptoSignerSign(BRCryptoSigner signer, BRCryptoKey key, byte[] signature, SizeT signatureLen, byte[] digest, SizeT digestLen);
    BRCryptoKey cryptoSignerRecover(BRCryptoSigner signer, byte[] digest, SizeT digestLen, byte[] signature, SizeT signatureLen);
    void cryptoSignerGive(BRCryptoSigner signer);

    // ethereum/util/BRUtilMath.h
    Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    // support/BRSyncMode.h
    Pointer BRSyncStoppedReasonGetMessage(BRSyncStoppedReason reason);
    Pointer BRDisconnectReasonGetMessage(BRDisconnectReason reason);
    Pointer BRTransferSubmitErrorGetMessage(BRTransferSubmitError error);
}
