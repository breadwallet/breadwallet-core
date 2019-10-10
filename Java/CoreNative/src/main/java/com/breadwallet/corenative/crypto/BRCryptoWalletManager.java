/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.support.BRSyncDepth;
import com.breadwallet.corenative.support.BRSyncMode;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.base.Optional;
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

public class BRCryptoWalletManager extends PointerType {

    public static Optional<BRCryptoWalletManager> create(BRCryptoCWMListener listener,
                                                         BRCryptoCWMClient client,
                                                         BRCryptoAccount account,
                                                         BRCryptoNetwork network,
                                                         BRSyncMode mode,
                                                         BRCryptoAddressScheme scheme,
                                                         String path) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoWalletManagerCreate(
                        listener.toByValue(),
                        client.toByValue(),
                        account,
                        network,
                        mode.toCore(),
                        scheme.toCore(),
                        path
                )
        );
    }

    public BRCryptoWalletManager(Pointer address) {
        super(address);
    }

    public BRCryptoWalletManager() {
        super();
    }

    public BRCryptoAccount getAccount() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetAccount(this);
    }

    public BRCryptoNetwork getNetwork() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetNetwork(this);
    }

    public BRCryptoWallet getWallet() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallet(this);
    }


    public List<BRCryptoWallet> getWallets() {
        List<BRCryptoWallet> wallets = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer walletsPtr = CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallets(this, count);
        if (null != walletsPtr) {
            try {
                int walletsSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer walletPtr : walletsPtr.getPointerArray(0, walletsSize)) {
                    wallets.add(new BRCryptoWallet.OwnedBRCryptoWallet(walletPtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(walletsPtr));
            }
        }
        return wallets;
    }

    public boolean containsWallet(BRCryptoWallet wallet) {
        return  BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoWalletManagerHasWallet(this, wallet);
    }

    public Optional<BRCryptoWallet> registerWallet(BRCryptoCurrency currency) {
        return Optional.fromNullable(
                CryptoLibrary.INSTANCE.cryptoWalletManagerRegisterWallet(this, currency)
        );
    }

    public void setNetworkReachable(boolean isNetworkReachable) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSetNetworkReachable(
                this,
                isNetworkReachable ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE
        );
    }

    public BRSyncMode getMode() {
        return BRSyncMode.fromCore(CryptoLibrary.INSTANCE.cryptoWalletManagerGetMode(this));
    }

    public void setMode(BRSyncMode mode) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSetMode(this, mode.toCore());
    }

    public String getPath() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetPath(this).getString(0, "UTF-8");
    }

    public BRCryptoWalletManagerState getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetState(this);
    }

    public BRCryptoAddressScheme getAddressScheme() {
        return BRCryptoAddressScheme.fromCore(CryptoLibrary.INSTANCE.cryptoWalletManagerGetAddressScheme(this));
    }

    public void setAddressScheme(BRCryptoAddressScheme scheme) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSetAddressScheme(this, scheme.toCore());
    }

    public void connect(BRCryptoPeer peer) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerConnect(this, peer);
    }

    public void disconnect() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerDisconnect(this);
    }

    public void sync() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSync(this);
    }

    public void stop() {
        CryptoLibrary.INSTANCE.cryptoWalletManagerStop(this);
    }

    public void syncToDepth(BRSyncDepth depth) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSyncToDepth(this, depth.toCore());
    }

    public void submit(BRCryptoWallet wallet, BRCryptoTransfer transfer, byte[] phraseUtf8) {
        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                CryptoLibrary.INSTANCE.cryptoWalletManagerSubmit(this, wallet, transfer, phraseBuffer);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public void submit(BRCryptoWallet wallet, BRCryptoTransfer transfer, BRCryptoKey key) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSubmitForKey(this, wallet, transfer, key);
    }

    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, UnsignedLong blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsInteger(this, callbackState, blockNumber.longValue());
    }

    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, String blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsString(this, callbackState, blockNumber);
    }

    public void announceGetBlockNumberFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberFailure(this, callbackState);
    }

    public void announceGetTransactionsItemBtc(BRCryptoCWMClientCallbackState callbackState, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemBTC(this, callbackState, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    public void announceGetTransactionsItemEth(BRCryptoCWMClientCallbackState callbackState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemETH(this, callbackState, hash, sourceAddr, targetAddr, contractAddr,
                amount, gasLimit, gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations,
                blockTransacionIndex, blockTimestamp, isError);
    }

    public void announceGetTransactionsItemGen(BRCryptoCWMClientCallbackState callbackState, byte[] transaction,
                                               UnsignedLong timestamp, UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemGEN(this, callbackState, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    public void announceGetTransactionsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccess(this, callbackState);
    }

    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState, String hash) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccessForHash(this, callbackState, hash);
    }

    public void announceSubmitTransferFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferFailure(this, callbackState);
    }

    public void announceGetBalanceSuccess(BRCryptoCWMClientCallbackState callbackState, String balance) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceSuccess(this, callbackState, balance);
    }

    public void announceGetBalanceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceFailure(this, callbackState);
    }

    public void announceGetGasPriceSuccess(BRCryptoCWMClientCallbackState callbackState, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceSuccess(this, callbackState, gasPrice);
    }

    public void announceGetGasPriceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceFailure(this, callbackState);
    }

    public void announceGetGasEstimateSuccess(BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateSuccess(this, callbackState, gasEstimate, gasPrice);
    }

    public void announceGetGasEstimateFailure(BRCryptoCWMClientCallbackState callbackState, BRCryptoStatus status) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateFailure(this, callbackState, status.toCore());
    }

    public void announceGetLogsItem(BRCryptoCWMClientCallbackState callbackState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        StringArray topicsArray = new StringArray(topics.toArray(new String[0]), "UTF-8");
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsItem(this, callbackState, hash, contract, topics.size(),
                topicsArray, data, gasPrice, gasUsed, logIndex,
                blockNumber, blockTransactionIndex, blockTimestamp);
    }

    public void announceGetLogsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceGetBlocksSuccess(BRCryptoCWMClientCallbackState callbackState, List<UnsignedLong> blocks) {
        int count = 0;
        long[] blockArray = new long[blocks.size()];
        for (UnsignedLong block: blocks) blockArray[count++] = block.longValue();
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksSuccess(this, callbackState, blockArray.length, blockArray);
    }

    public void announceGetBlocksFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksFailure(this, callbackState);
    }

    public void announceGetTokensItem(BRCryptoCWMClientCallbackState callbackState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensItem(this, callbackState, address, symbol, name, description,
                decimals.intValue(), gasLimit, gasPrice);
    }

    public void announceGetTokensComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensComplete(this, callbackState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceGetNonceSuccess(BRCryptoCWMClientCallbackState callbackState, String address, String nonce) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceSuccess(this, callbackState, address, nonce);
    }

    public void announceGetNonceFailure(BRCryptoCWMClientCallbackState callbackState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceFailure(this, callbackState);
    }

    public BRCryptoWalletManager take() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerTake(this);
    }

    public void give() {
        if (null != getPointer()) {
            CryptoLibrary.INSTANCE.cryptoWalletManagerGive(this);
        }
    }
}
