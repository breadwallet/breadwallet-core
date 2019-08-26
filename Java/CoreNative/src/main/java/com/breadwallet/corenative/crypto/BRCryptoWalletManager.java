/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedInts;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class BRCryptoWalletManager extends PointerType implements CoreBRCryptoWalletManager {

    public BRCryptoWalletManager(Pointer address) {
        super(address);
    }

    public BRCryptoWalletManager() {
        super();
    }

    @Override
    public CoreBRCryptoAccount getAccount() {
        return new OwnedBRCryptoAccount(CryptoLibrary.INSTANCE.cryptoWalletManagerGetAccount(this));
    }

    @Override
    public CoreBRCryptoNetwork getNetwork() {
        return new OwnedBRCryptoNetwork(CryptoLibrary.INSTANCE.cryptoWalletManagerGetNetwork(this));
    }

    @Override
    public CoreBRCryptoWallet getWallet() {
        return new OwnedBRCryptoWallet(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallet(this));
    }


    @Override
    public List<CoreBRCryptoWallet> getWallets() {
        List<CoreBRCryptoWallet> wallets = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer walletsPtr = CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallets(this, count);
        if (null != walletsPtr) {
            try {
                int walletsSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer walletPtr : walletsPtr.getPointerArray(0, walletsSize)) {
                    wallets.add(new OwnedBRCryptoWallet(new BRCryptoWallet(walletPtr)));
                }

            } finally {
                Native.free(Pointer.nativeValue(walletsPtr));
            }
        }
        return wallets;
    }

    @Override
    public boolean containsWallet(CoreBRCryptoWallet wallet) {
        return  BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoWalletManagerHasWallet(this, wallet.asBRCryptoWallet());
    }

    @Override
    public int getMode() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetMode(this);
    }

    @Override
    public void setMode(int mode) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSetMode(this, mode);
    }

    @Override
    public String getPath() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetPath(this).getString(0, "UTF-8");
    }

    @Override
    public int getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetState(this);
    }

    @Override
    public int getAddressScheme() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetAddressScheme(this);
    }

    @Override
    public void setAddressScheme(int scheme) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSetAddressScheme(this, scheme);
    }

    @Override
    public void connect() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerConnect(this);
    }

    @Override
    public void disconnect() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerDisconnect(this);
    }

    @Override
    public void sync() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSync(this);
    }

    @Override
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, byte[] phraseUtf8) {
        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                CryptoLibrary.INSTANCE.cryptoWalletManagerSubmit(this, wallet.asBRCryptoWallet(), transfer.asBRCryptoTransfer(), phraseBuffer);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, UnsignedLong blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsInteger(this, callbackState, blockNumber.longValue());
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, String blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsString(this, callbackState, blockNumber);
    }

    @Override
    public void announceGetBlockNumberFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberFailure(this, callbackState);
    }

    @Override
    public void announceGetTransactionsItemBtc(BRCryptoCWMClientCallbackState callbackState, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemBTC(this, callbackState, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    @Override
    public void announceGetTransactionsItemEth(BRCryptoCWMClientCallbackState callbackState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemETH(this, callbackState, hash, sourceAddr, targetAddr, contractAddr,
                amount, gasLimit, gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations,
                blockTransacionIndex, blockTimestamp, isError);
    }

    @Override
    public void announceGetTransactionsItemGen(BRCryptoCWMClientCallbackState callbackState, byte[] transaction,
                                               UnsignedLong timestamp, UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemGEN(this, callbackState, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    @Override
    public void announceGetTransactionsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccess(this, callbackState);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState, String hash) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccessForHash(this, callbackState, hash);
    }

    @Override
    public void announceSubmitTransferFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferFailure(this, callbackState);
    }

    @Override
    public void announceGetBalanceSuccess(BRCryptoCWMClientCallbackState callbackState, String balance) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceSuccess(this, callbackState, balance);
    }

    @Override
    public void announceGetBalanceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceFailure(this, callbackState);
    }

    @Override
    public void announceGetGasPriceSuccess(BRCryptoCWMClientCallbackState callbackState, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceSuccess(this, callbackState, gasPrice);
    }

    @Override
    public void announceGetGasPriceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceFailure(this, callbackState);
    }

    @Override
    public void announceGetGasEstimateSuccess(BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateSuccess(this, callbackState, gasEstimate, gasPrice);
    }

    @Override
    public void announceGetGasEstimateFailure(BRCryptoCWMClientCallbackState callbackState, int status) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateFailure(this, callbackState, status);
    }

    @Override
    public void announceGetLogsItem(BRCryptoCWMClientCallbackState callbackState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        StringArray topicsArray = new StringArray(topics.toArray(new String[0]), "UTF-8");
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsItem(this, callbackState, hash, contract, topics.size(),
                topicsArray, data, gasPrice, gasUsed, logIndex,
                blockNumber, blockTransactionIndex, blockTimestamp);
    }

    @Override
    public void announceGetLogsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceGetBlocksSuccess(BRCryptoCWMClientCallbackState callbackState, List<UnsignedLong> blocks) {
        int count = 0;
        long[] blockArray = new long[blocks.size()];
        for (UnsignedLong block: blocks) blockArray[count++] = block.longValue();
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksSuccess(this, callbackState, blockArray.length, blockArray);
    }

    @Override
    public void announceGetBlocksFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksFailure(this, callbackState);
    }

    @Override
    public void announceGetTokensItem(BRCryptoCWMClientCallbackState callbackState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensItem(this, callbackState, address, symbol, name, description,
                decimals.intValue(), gasLimit, gasPrice);
    }

    @Override
    public void announceGetTokensComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceGetNonceSuccess(BRCryptoCWMClientCallbackState callbackState, String address, String nonce) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceSuccess(this, callbackState, address, nonce);
    }

    @Override
    public void announceGetNonceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceFailure(this, callbackState);
    }
}
