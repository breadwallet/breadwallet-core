/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/18/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoTransferState;
import com.breadwallet.corenative.crypto.BRCryptoWalletMigratorStatus;
import com.breadwallet.corenative.support.BRDisconnectReason;
import com.breadwallet.corenative.support.BRSyncStoppedReason;
import com.breadwallet.corenative.support.BRTransferSubmitError;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;

import java.nio.ByteBuffer;

public final class CryptoLibraryDirect {

    //
    // Crypto Core
    //

    // crypto/BRCryptoAccount.h
    public static native Pointer cryptoAccountCreate(ByteBuffer phrase, long timestamp);
    public static native Pointer cryptoAccountCreateFromSerialization(byte[] serialization, SizeT serializationLength);
    public static native long cryptoAccountGetTimestamp(Pointer account);
    public static native Pointer cryptoAccountGetFileSystemIdentifier(Pointer account);
    public static native Pointer cryptoAccountSerialize(Pointer account, SizeTByReference count);
    public static native int cryptoAccountValidateSerialization(Pointer account, byte[] serialization, SizeT count);
    public static native int cryptoAccountValidateWordsList(SizeT count);
    public static native Pointer cryptoAccountGeneratePaperKey(StringArray words);
    public static native int cryptoAccountValidatePaperKey(ByteBuffer phraseBuffer, StringArray wordsArray);
    public static native void cryptoAccountGive(Pointer obj);

    // crypto/BRCryptoAddress.h
    public static native Pointer cryptoAddressAsString(Pointer address);
    public static native int cryptoAddressIsIdentical(Pointer a1, Pointer a2);
    public static native void cryptoAddressGive(Pointer obj);

    // crypto/BRCryptoAmount.h
    public static native Pointer cryptoAmountCreateDouble(double value, Pointer unit);
    public static native Pointer cryptoAmountCreateInteger(long value, Pointer unit);
    public static native Pointer cryptoAmountCreateString(String value, int isNegative, Pointer unit);
    public static native Pointer cryptoAmountGetCurrency(Pointer amount);
    public static native Pointer cryptoAmountGetUnit(Pointer amount);
    public static native int cryptoAmountHasCurrency(Pointer amount, Pointer currency);
    public static native int cryptoAmountIsNegative(Pointer amount);
    public static native int cryptoAmountIsCompatible(Pointer a1, Pointer a2);
    public static native int cryptoAmountCompare(Pointer a1, Pointer a2);
    public static native Pointer cryptoAmountAdd(Pointer a1, Pointer a2);
    public static native Pointer cryptoAmountSub(Pointer a1, Pointer a2);
    public static native Pointer cryptoAmountNegate(Pointer amount);
    public static native Pointer cryptoAmountConvertToUnit(Pointer amount, Pointer unit);
    public static native double cryptoAmountGetDouble(Pointer amount, Pointer unit, IntByReference overflow);
    public static native UInt256.ByValue cryptoAmountGetValue(Pointer amount);
    public static native void cryptoAmountGive(Pointer obj);

    // crypto/BRCryptoCurrency.h
    public static native Pointer cryptoCurrencyGetUids(Pointer currency);
    public static native Pointer cryptoCurrencyGetName(Pointer currency);
    public static native Pointer cryptoCurrencyGetCode(Pointer currency);
    public static native Pointer cryptoCurrencyGetType(Pointer currency);
    public static native Pointer cryptoCurrencyGetIssuer(Pointer currency);
    public static native int cryptoCurrencyIsIdentical(Pointer c1, Pointer c2);
    public static native void cryptoCurrencyGive(Pointer obj);

    // crypto/BRCryptoFeeBasis.h
    public static native Pointer cryptoFeeBasisGetPricePerCostFactor (Pointer feeBasis);
    public static native Pointer cryptoFeeBasisGetPricePerCostFactorUnit (Pointer feeBasis);
    public static native double cryptoFeeBasisGetCostFactor (Pointer feeBasis);
    public static native Pointer cryptoFeeBasisGetFee (Pointer feeBasis);
    public static native int cryptoFeeBasisIsIdentical(Pointer f1, Pointer f2);
    public static native void cryptoFeeBasisGive(Pointer obj);

    // crypto/BRCryptoHash.h
    public static native int cryptoHashEqual(Pointer h1, Pointer h2);
    public static native Pointer cryptoHashString(Pointer hash);
    public static native int cryptoHashGetHashValue(Pointer hash);
    public static native void cryptoHashGive(Pointer obj);

    // crypto/BRCryptoKey.h
    public static native int cryptoKeyIsProtectedPrivate(ByteBuffer keyBuffer);
    public static native Pointer cryptoKeyCreateFromPhraseWithWords(ByteBuffer phraseBuffer, StringArray wordsArray);
    public static native Pointer cryptoKeyCreateFromStringPrivate(ByteBuffer stringBuffer);
    public static native Pointer cryptoKeyCreateFromStringProtectedPrivate(ByteBuffer stringBuffer, ByteBuffer phraseBuffer);
    public static native Pointer cryptoKeyCreateFromStringPublic(ByteBuffer stringBuffer);
    public static native Pointer cryptoKeyCreateForPigeon(Pointer key, byte[] nonce, SizeT nonceCount);
    public static native Pointer cryptoKeyCreateForBIP32ApiAuth(ByteBuffer phraseBuffer, StringArray wordsArray);
    public static native Pointer cryptoKeyCreateForBIP32BitID(ByteBuffer phraseBuffer, int index, String uri, StringArray wordsArray);
    public static native Pointer cryptoKeyCreateFromSecret(UInt256.ByValue secret);
    public static native void cryptoKeyProvidePublicKey(Pointer key, int useCompressed, int compressed);
    public static native int cryptoKeyHasSecret(Pointer key);
    public static native int cryptoKeyPublicMatch(Pointer key, Pointer other);
    public static native int cryptoKeySecretMatch(Pointer key, Pointer other);
    public static native Pointer cryptoKeyEncodePrivate(Pointer key);
    public static native Pointer cryptoKeyEncodePublic(Pointer key);
    public static native UInt256.ByValue cryptoKeyGetSecret(Pointer key);
    public static native void cryptoKeyGive(Pointer key);

    // crypto/BRCryptoNetwork.h
    public static native Pointer cryptoNetworkGetUids(Pointer network);
    public static native Pointer cryptoNetworkGetName(Pointer network);
    public static native int cryptoNetworkIsMainnet(Pointer network);
    public static native Pointer cryptoNetworkGetCurrency(Pointer network);
    public static native Pointer cryptoNetworkGetUnitAsDefault(Pointer network, Pointer currency);
    public static native Pointer cryptoNetworkGetUnitAsBase(Pointer network, Pointer currency);
    public static native long cryptoNetworkGetHeight(Pointer network);
    public static native int cryptoNetworkGetConfirmationsUntilFinal(Pointer network);
    public static native void cryptoNetworkSetConfirmationsUntilFinal(Pointer network, int confirmationsUntilFinal);
    public static native SizeT cryptoNetworkGetCurrencyCount(Pointer network);
    public static native Pointer cryptoNetworkGetCurrencyAt(Pointer network, SizeT index);
    public static native int cryptoNetworkHasCurrency(Pointer network, Pointer currency);
    public static native SizeT cryptoNetworkGetUnitCount(Pointer network, Pointer currency);
    public static native Pointer cryptoNetworkGetUnitAt(Pointer network, Pointer currency, SizeT index);
    // public static native void cryptoNetworkSetNetworkFees(Pointer network, BRCryptoNetworkFee[] fees, SizeT count);
    public static native Pointer cryptoNetworkGetNetworkFees(Pointer network, SizeTByReference count);
    public static native Pointer cryptoNetworkCreateAddressFromString(Pointer network, String address);
    public static native Pointer cryptoNetworkTake(Pointer obj);
    public static native void cryptoNetworkGive(Pointer obj);

    // crypto/BRCryptoNetwork.h (BRCryptoNetworkFee)
    public static native long cryptoNetworkFeeGetConfirmationTimeInMilliseconds(Pointer fee);
    public static native int cryptoNetworkFeeEqual(Pointer fee, Pointer other);
    public static native void cryptoNetworkFeeGive(Pointer obj);

    // crypto/BRCryptoNetwork.h (BRCryptoPeer)
    public static native Pointer cryptoPeerCreate(Pointer network, String address, short port, String publicKey);
    public static native Pointer cryptoPeerGetNetwork(Pointer peer);
    public static native Pointer cryptoPeerGetAddress(Pointer peer);
    public static native Pointer cryptoPeerGetPublicKey(Pointer peer);
    public static native short cryptoPeerGetPort(Pointer peer);
    public static native int cryptoPeerIsIdentical(Pointer peer, Pointer other);
    public static native void cryptoPeerGive(Pointer peer);

    // crypto/BRCryptoPrivate.h (BRCryptoCurrency)
    public static native Pointer cryptoCurrencyCreate(String uids, String name, String code, String type, String issuer);

    // crypto/BRCryptoPrivate.h (BRCryptoNetworkFee)
    public static native Pointer cryptoNetworkFeeCreate(long timeInternalInMilliseconds, Pointer pricePerCostFactor, Pointer pricePerCostFactorUnit);

    // crypto/BRCryptoPrivate.h (BRCryptoNetwork)
    public static native Pointer cryptoNetworkCreateAsBTC(String uids, String name, byte forkId, Pointer params);
    public static native Pointer cryptoNetworkCreateAsETH(String uids, String name, int chainId, Pointer net);
    public static native Pointer cryptoNetworkCreateAsGEN(String uids, String name, byte isMainnet);
    public static native void cryptoNetworkSetHeight(Pointer network, long height);
    public static native void cryptoNetworkSetCurrency(Pointer network, Pointer currency);
    public static native void cryptoNetworkAddCurrency(Pointer network, Pointer currency, Pointer baseUnit, Pointer defaultUnit);
    public static native void cryptoNetworkAddCurrencyUnit(Pointer network, Pointer currency, Pointer unit);
    public static native void cryptoNetworkAddNetworkFee(Pointer network, Pointer networkFee);

    // crypto/BRCryptoPrivate.h (BRCryptoUnit)
    public static native Pointer cryptoUnitCreateAsBase(Pointer currency, String uids, String name, String symbol);
    public static native Pointer cryptoUnitCreate(Pointer currency, String uids, String name, String symbol, Pointer base, byte decimals);

    // crypto/BRCryptoTransfer.h
    public static native Pointer cryptoTransferGetSourceAddress(Pointer transfer);
    public static native Pointer cryptoTransferGetTargetAddress(Pointer transfer);
    public static native Pointer cryptoTransferGetAmount(Pointer transfer);
    public static native Pointer cryptoTransferGetAmountDirected(Pointer transfer);
    public static native int cryptoTransferGetDirection(Pointer transfer);
    public static native BRCryptoTransferState.ByValue cryptoTransferGetState(Pointer transfer);
    public static native Pointer cryptoTransferGetHash(Pointer transfer);
    public static native Pointer cryptoTransferGetUnitForAmount (Pointer transfer);
    public static native Pointer cryptoTransferGetUnitForFee (Pointer transfer);
    public static native Pointer cryptoTransferGetEstimatedFeeBasis (Pointer transfer);
    public static native Pointer cryptoTransferGetConfirmedFeeBasis (Pointer transfer);
    public static native int cryptoTransferEqual(Pointer transfer, Pointer other);
    public static native Pointer cryptoTransferTake(Pointer obj);
    public static native void cryptoTransferGive(Pointer obj);

    // crypto/BRCryptoUnit.h
    public static native Pointer cryptoUnitGetUids(Pointer unit);
    public static native Pointer cryptoUnitGetName(Pointer unit);
    public static native Pointer cryptoUnitGetSymbol(Pointer unit);
    public static native Pointer cryptoUnitGetCurrency(Pointer unit);
    public static native int cryptoUnitHasCurrency(Pointer unit, Pointer currency);
    public static native Pointer cryptoUnitGetBaseUnit(Pointer unit);
    public static native byte cryptoUnitGetBaseDecimalOffset(Pointer unit);
    public static native int cryptoUnitIsCompatible(Pointer u1, Pointer u2);
    public static native int cryptoUnitIsIdentical(Pointer u1, Pointer u2);
    public static native void cryptoUnitGive(Pointer obj);

    // crypto/BRCryptoWallet.h
    public static native int cryptoWalletGetState(Pointer wallet);
    public static native Pointer cryptoWalletGetBalance(Pointer wallet);
    public static native Pointer cryptoWalletGetTransfers(Pointer wallet, SizeTByReference count);
    public static native int cryptoWalletHasTransfer(Pointer wallet, Pointer transfer);
    public static native Pointer cryptoWalletGetAddress(Pointer wallet, int addressScheme);
    public static native Pointer cryptoWalletGetUnit(Pointer wallet);
    public static native Pointer cryptoWalletGetUnitForFee(Pointer wallet);
    public static native Pointer cryptoWalletGetCurrency(Pointer wallet);
    public static native Pointer cryptoWalletCreateTransfer(Pointer wallet, Pointer target, Pointer amount, Pointer feeBasis);
    public static native Pointer cryptoWalletCreateTransferForWalletSweep(Pointer wallet, Pointer sweeper, Pointer feeBasis);
    public static native void cryptoWalletEstimateFeeBasis(Pointer wallet, Pointer cookie, Pointer target, Pointer amount, Pointer fee);
    public static native void cryptoWalletEstimateFeeBasisForWalletSweep(Pointer wallet, Pointer cookie, Pointer sweeper, Pointer fee);
    public static native Pointer cryptoWalletTake(Pointer wallet);
    public static native void cryptoWalletGive(Pointer obj);

    // crypto/BRCryptoWalletManager.h (BRCryptoWalletMigrator)
    public static native Pointer cryptoWalletMigratorCreate(Pointer network, String storagePath);
    public static native BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandleTransactionAsBTC (Pointer migrator, byte[] bytes, SizeT bytesCount, int blockHeight, int timestamp);
    public static native BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandleBlockBytesAsBTC(Pointer migrator, byte[] buffer, SizeT bufferLen, int height);
    public static native BRCryptoWalletMigratorStatus.ByValue cryptoWalletMigratorHandlePeerAsBTC (Pointer migrator, int address, short port, long services, int timestamp);
    public static native void cryptoWalletMigratorRelease(Pointer migrator);

    // crypto/BRCryptoWalletManager.h (BRCryptoWalletSweeper)
    public static native int cryptoWalletSweeperValidateSupported(Pointer network, Pointer currency, Pointer key, Pointer wallet);
    public static native Pointer cryptoWalletSweeperCreateAsBtc(Pointer network, Pointer currency, Pointer key, int scheme);
    public static native Pointer cryptoWalletSweeperGetKey(Pointer sweeper);
    public static native Pointer cryptoWalletSweeperGetBalance(Pointer sweeper);
    public static native Pointer cryptoWalletSweeperGetAddress(Pointer sweeper);
    public static native int cryptoWalletSweeperHandleTransactionAsBTC(Pointer sweeper, byte[] transaction, SizeT transactionLen);
    public static native int cryptoWalletSweeperValidate(Pointer sweeper);
    public static native void cryptoWalletSweeperRelease(Pointer sweeper);

    //
    // Crypto Primitives
    //

    // crypto/BRCryptoCipher.h
    public static native Pointer cryptoCipherCreateForAESECB(byte[] key, SizeT keyLen);
    public static native Pointer cryptoCipherCreateForChacha20Poly1305(Pointer key, byte[] nonce12, SizeT nonce12Len, byte[] ad, SizeT adLen);
    public static native Pointer cryptoCipherCreateForPigeon(Pointer privKey, Pointer pubKey, byte[] nonce12, SizeT nonce12Len);
    public static native SizeT cryptoCipherEncryptLength(Pointer cipher, byte[] src, SizeT srcLen);
    public static native int cryptoCipherEncrypt(Pointer cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native SizeT cryptoCipherDecryptLength(Pointer cipher, byte[] src, SizeT srcLen);
    public static native int cryptoCipherDecrypt(Pointer cipher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native void cryptoCipherGive(Pointer cipher);

    // crypto/BRCryptoCoder.h
    public static native Pointer cryptoCoderCreate(int type);
    public static native SizeT cryptoCoderEncodeLength(Pointer coder, byte[] src, SizeT srcLen);
    public static native int cryptoCoderEncode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native SizeT cryptoCoderDecodeLength(Pointer coder, byte[] src);
    public static native int cryptoCoderDecode(Pointer coder, byte[] dst, SizeT dstLen, byte[] src);
    public static native void cryptoCoderGive(Pointer coder);

    // crypto/BRCryptoHasher.h
    public static native Pointer cryptoHasherCreate(int type);
    public static native SizeT cryptoHasherLength(Pointer hasher);
    public static native int cryptoHasherHash(Pointer hasher, byte[] dst, SizeT dstLen, byte[] src, SizeT srcLen);
    public static native void cryptoHasherGive(Pointer hasher);

    // crypto/BRCryptoSigner.h
    public static native Pointer cryptoSignerCreate(int type);
    public static native SizeT cryptoSignerSignLength(Pointer signer, Pointer key, byte[] digest, SizeT digestlen);
    public static native int cryptoSignerSign(Pointer signer, Pointer key, byte[] signature, SizeT signatureLen, byte[] digest, SizeT digestLen);
    public static native Pointer cryptoSignerRecover(Pointer signer, byte[] digest, SizeT digestLen, byte[] signature, SizeT signatureLen);
    public static native void cryptoSignerGive(Pointer signer);

    //
    // Support
    //

    // support/BRSyncMode.h
    public static native Pointer BRSyncStoppedReasonGetMessage(BRSyncStoppedReason reason);
    public static native Pointer BRDisconnectReasonGetMessage(BRDisconnectReason reason);
    public static native Pointer BRTransferSubmitErrorGetMessage(BRTransferSubmitError error);

    //
    // Ethereum
    //


    // ethereum/util/BRUtilMath.h
    public static native Pointer coerceStringPrefaced(UInt256.ByValue value, int base, String preface);

    static {
        Native.register(CryptoLibraryDirect.class, CryptoLibrary.LIBRARY);
    }

    private CryptoLibraryDirect() {}
}
