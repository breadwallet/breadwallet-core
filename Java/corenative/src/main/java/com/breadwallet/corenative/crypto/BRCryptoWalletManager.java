/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.breadwallet.corenative.CryptoLibraryIndirect;
import com.breadwallet.corenative.utility.Cookie;
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
import com.sun.jna.ptr.IntByReference;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

import javax.annotation.Nullable;

public class BRCryptoWalletManager extends PointerType {

    public static void wipe(BRCryptoNetwork network, String path) {
        CryptoLibraryDirect.cryptoWalletManagerWipe(network.getPointer(), path);
    }

    public static Optional<BRCryptoWalletManager> create(BRCryptoCWMListener listener,
                                                         BRCryptoCWMClient client,
                                                         BRCryptoAccount account,
                                                         BRCryptoNetwork network,
                                                         BRCryptoSyncMode mode,
                                                         BRCryptoAddressScheme scheme,
                                                         String path) {
        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletManagerCreate(
                        listener.toByValue(),
                        client.toByValue(),
                        account.getPointer(),
                        network.getPointer(),
                        mode.toCore(),
                        scheme.toCore(),
                        path
                )
        ).transform(BRCryptoWalletManager::new);
    }

    public BRCryptoWalletManager() {
        super();
    }

    public BRCryptoWalletManager(Pointer address) {
        super(address);
    }

    public BRCryptoAccount getAccount() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoAccount(CryptoLibraryDirect.cryptoWalletManagerGetAccount(thisPtr));
    }

    public BRCryptoNetwork getNetwork() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoNetwork(CryptoLibraryDirect.cryptoWalletManagerGetNetwork(thisPtr));
    }

    public BRCryptoWallet getWallet() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoWallet(CryptoLibraryDirect.cryptoWalletManagerGetWallet(thisPtr));
    }


    public List<BRCryptoWallet> getWallets() {
        Pointer thisPtr = this.getPointer();

        List<BRCryptoWallet> wallets = new ArrayList<>();
        SizeTByReference count = new SizeTByReference();
        Pointer walletsPtr = CryptoLibraryDirect.cryptoWalletManagerGetWallets(thisPtr, count);
        if (null != walletsPtr) {
            try {
                int walletsSize = UnsignedInts.checkedCast(count.getValue().longValue());
                for (Pointer walletPtr : walletsPtr.getPointerArray(0, walletsSize)) {
                    wallets.add(new BRCryptoWallet(walletPtr));
                }

            } finally {
                Native.free(Pointer.nativeValue(walletsPtr));
            }
        }
        return wallets;
    }

    public boolean containsWallet(BRCryptoWallet wallet) {
        Pointer thisPtr = this.getPointer();

        return  BRCryptoBoolean.CRYPTO_TRUE == CryptoLibraryDirect.cryptoWalletManagerHasWallet(thisPtr, wallet.getPointer());
    }

    public Optional<BRCryptoWallet> registerWallet(BRCryptoCurrency currency) {
        Pointer thisPtr = this.getPointer();

        return Optional.fromNullable(
                CryptoLibraryDirect.cryptoWalletManagerRegisterWallet(
                        thisPtr,
                        currency.getPointer()
                )
        ).transform(BRCryptoWallet::new);
    }

    public void setNetworkReachable(boolean isNetworkReachable) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSetNetworkReachable(
                thisPtr,
                isNetworkReachable ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE
        );
    }

    public BRCryptoSyncMode getMode() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoSyncMode.fromCore(CryptoLibraryDirect.cryptoWalletManagerGetMode(thisPtr));
    }

    public void setMode(BRCryptoSyncMode mode) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSetMode(thisPtr, mode.toCore());
    }

    public String getPath() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoWalletManagerGetPath(thisPtr).getString(0, "UTF-8");
    }

    public BRCryptoWalletManagerState getState() {
        Pointer thisPtr = this.getPointer();

        return CryptoLibraryDirect.cryptoWalletManagerGetState(thisPtr);
    }

    public BRCryptoAddressScheme getAddressScheme() {
        Pointer thisPtr = this.getPointer();

        return BRCryptoAddressScheme.fromCore(CryptoLibraryDirect.cryptoWalletManagerGetAddressScheme(thisPtr));
    }

    public void setAddressScheme(BRCryptoAddressScheme scheme) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSetAddressScheme(thisPtr, scheme.toCore());
    }

    public void connect(@Nullable BRCryptoPeer peer) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerConnect(thisPtr, peer == null ? null : peer.getPointer());
    }

    public void disconnect() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerDisconnect(thisPtr);
    }

    public void sync() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSync(thisPtr);
    }

    public void stop() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerStop(thisPtr);
    }

    public void syncToDepth(BRCryptoSyncDepth depth) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSyncToDepth(thisPtr, depth.toCore());
    }

    public boolean sign(BRCryptoWallet wallet, BRCryptoTransfer transfer, byte[] phraseUtf8) {
        Pointer thisPtr = this.getPointer();

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                int success = CryptoLibraryDirect.cryptoWalletManagerSign(thisPtr, wallet.getPointer(), transfer.getPointer(), phraseBuffer);
                return BRCryptoBoolean.CRYPTO_TRUE == success;
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public void submit(BRCryptoWallet wallet, BRCryptoTransfer transfer, byte[] phraseUtf8) {
        Pointer thisPtr = this.getPointer();

        // ensure string is null terminated
        phraseUtf8 = Arrays.copyOf(phraseUtf8, phraseUtf8.length + 1);
        try {
            Memory phraseMemory = new Memory(phraseUtf8.length);
            try {
                phraseMemory.write(0, phraseUtf8, 0, phraseUtf8.length);
                ByteBuffer phraseBuffer = phraseMemory.getByteBuffer(0, phraseUtf8.length);

                CryptoLibraryDirect.cryptoWalletManagerSubmit(thisPtr, wallet.getPointer(), transfer.getPointer(), phraseBuffer);
            } finally {
                phraseMemory.clear();
            }
        } finally {
            // clear out our copy; caller responsible for original array
            Arrays.fill(phraseUtf8, (byte) 0);
        }
    }

    public void submit(BRCryptoWallet wallet, BRCryptoTransfer transfer, BRCryptoKey key) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSubmitForKey(thisPtr, wallet.getPointer(), transfer.getPointer(), key.getPointer());
    }

    public void submit(BRCryptoWallet wallet, BRCryptoTransfer transfer) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerSubmitSigned(thisPtr, wallet.getPointer(), transfer.getPointer());
    }

    public static class EstimateLimitResult {

        public @Nullable BRCryptoAmount amount;
        public boolean needFeeEstimate;
        public boolean isZeroIfInsuffientFunds;

        EstimateLimitResult(@Nullable BRCryptoAmount amount, boolean needFeeEstimate, boolean isZeroIfInsuffientFunds) {
            this.amount = amount;
            this.needFeeEstimate = needFeeEstimate;
            this.isZeroIfInsuffientFunds = isZeroIfInsuffientFunds;
        }
    }

    public EstimateLimitResult estimateLimit(BRCryptoWallet wallet, boolean asMaximum, BRCryptoAddress coreAddress, BRCryptoNetworkFee coreFee) {
        Pointer thisPtr = this.getPointer();

        IntByReference needFeeEstimateRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        IntByReference isZeroIfInsuffientFundsRef = new IntByReference(BRCryptoBoolean.CRYPTO_FALSE);
        Optional<BRCryptoAmount> maybeAmount = Optional.fromNullable(CryptoLibraryDirect.cryptoWalletManagerEstimateLimit(
                thisPtr,
                wallet.getPointer(),
                asMaximum ? BRCryptoBoolean.CRYPTO_TRUE : BRCryptoBoolean.CRYPTO_FALSE,
                coreAddress.getPointer(),
                coreFee.getPointer(),
                needFeeEstimateRef,
                isZeroIfInsuffientFundsRef
        )).transform(BRCryptoAmount::new);

        return new EstimateLimitResult(
                maybeAmount.orNull(),
                needFeeEstimateRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE,
                isZeroIfInsuffientFundsRef.getValue() == BRCryptoBoolean.CRYPTO_TRUE
        );
    }

    public void estimateFeeBasis(BRCryptoWallet wallet, Cookie cookie,
                                 BRCryptoAddress target, BRCryptoAmount amount, BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerEstimateFeeBasis(
                thisPtr,
                wallet.getPointer(),
                cookie.getPointer(),
                target.getPointer(),
                amount.getPointer(),
                fee.getPointer());
    }

    public void estimateFeeBasisForWalletSweep(BRCryptoWallet wallet, Cookie cookie,
                                               BRCryptoWalletSweeper sweeper, BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerEstimateFeeBasisForWalletSweep(
                thisPtr,
                wallet.getPointer(),
                cookie.getPointer(),
                sweeper.getPointer(),
                fee.getPointer());
    }

    public void estimateFeeBasisForPaymentProtocolRequest(BRCryptoWallet wallet, Cookie cookie,
                                                          BRCryptoPaymentProtocolRequest request, BRCryptoNetworkFee fee) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerEstimateFeeBasisForPaymentProtocolRequest(
                thisPtr,
                wallet.getPointer(),
                cookie.getPointer(),
                request.getPointer(),
                fee.getPointer());
    }

    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, UnsignedLong blockNumber) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBlockNumberSuccessAsInteger(thisPtr, callbackState.getPointer(), blockNumber.longValue());
    }

    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, String blockNumber) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBlockNumberSuccessAsString(thisPtr, callbackState.getPointer(), blockNumber);
    }

    public void announceGetBlockNumberFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBlockNumberFailure(thisPtr, callbackState.getPointer());
    }

    public void announceGetTransactionsItemBtc(BRCryptoCWMClientCallbackState callbackState, BRCryptoTransferStateType status, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTransactionsItemBTC(thisPtr, callbackState.getPointer(), status.toCore(), transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    public void announceGetTransactionsItemEth(BRCryptoCWMClientCallbackState callbackState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTransactionsItemETH(thisPtr, callbackState.getPointer(), hash, sourceAddr, targetAddr, contractAddr,
                amount, gasLimit, gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations,
                blockTransacionIndex, blockTimestamp, isError);
    }

    public void announceGetTransactionsItemGen(BRCryptoCWMClientCallbackState callbackState, BRCryptoTransferStateType status, byte[] transaction,
                                               UnsignedLong timestamp, UnsignedLong blockHeight) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTransactionsItemGEN(thisPtr, callbackState.getPointer(), status.toCore(), transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    public void announceGetTransactionsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTransactionsComplete(thisPtr, callbackState.getPointer(), success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceGetTransfersItemGen(BRCryptoCWMClientCallbackState callbackState, BRCryptoTransferStateType status,
                                            String hash,
                                            String uids,
                                            String from,
                                            String to,
                                            String amount,
                                            String currency,
                                            String fee,
                                            UnsignedLong timestamp,
                                            UnsignedLong blockHeight,
                                            Map<String, String> meta) {
        Pointer thisPtr = this.getPointer();

        int metaCount = meta.size();
        String[] metaKeys = meta.keySet().toArray(new String[metaCount]);
        String[] metaVals = meta.values().toArray(new String[metaCount]);

        CryptoLibraryIndirect.cwmAnnounceGetTransferItemGEN(
                thisPtr,
                callbackState.getPointer(),
                status.toCore(),
                hash,
                uids,
                from,
                to,
                amount,
                currency,
                fee,
                timestamp.longValue(),
                blockHeight.longValue(),
                new SizeT(metaCount),
                metaKeys,
                metaVals);
    }

    public void announceGetTransfersComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTransfersComplete(thisPtr, callbackState.getPointer(), success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceSubmitTransferSuccess(thisPtr, callbackState.getPointer());
    }

    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState, String hash) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceSubmitTransferSuccessForHash(thisPtr, callbackState.getPointer(), hash);
    }

    public void announceSubmitTransferFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceSubmitTransferFailure(thisPtr, callbackState.getPointer());
    }

    public void announceGetBalanceSuccess(BRCryptoCWMClientCallbackState callbackState, String balance) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBalanceSuccess(thisPtr, callbackState.getPointer(), balance);
    }

    public void announceGetBalanceFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBalanceFailure(thisPtr, callbackState.getPointer());
    }

    public void announceGetGasPriceSuccess(BRCryptoCWMClientCallbackState callbackState, String gasPrice) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetGasPriceSuccess(thisPtr, callbackState.getPointer(), gasPrice);
    }

    public void announceGetGasPriceFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetGasPriceFailure(thisPtr, callbackState.getPointer());
    }

    public void announceGetGasEstimateSuccess(BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetGasEstimateSuccess(thisPtr, callbackState.getPointer(), gasEstimate, gasPrice);
    }

    public void announceGetGasEstimateFailure(BRCryptoCWMClientCallbackState callbackState, BRCryptoStatus status) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetGasEstimateFailure(thisPtr, callbackState.getPointer(), status.toCore());
    }

    public void announceGetLogsItem(BRCryptoCWMClientCallbackState callbackState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        Pointer thisPtr = this.getPointer();

        StringArray topicsArray = new StringArray(topics.toArray(new String[0]), "UTF-8");
        CryptoLibraryDirect.cwmAnnounceGetLogsItem(thisPtr, callbackState.getPointer(), hash, contract, topics.size(),
                topicsArray, data, gasPrice, gasUsed, logIndex,
                blockNumber, blockTransactionIndex, blockTimestamp);
    }

    public void announceGetLogsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetLogsComplete(thisPtr, callbackState.getPointer(), success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceGetBlocksSuccess(BRCryptoCWMClientCallbackState callbackState, List<UnsignedLong> blocks) {
        Pointer thisPtr = this.getPointer();

        int count = 0;
        long[] blockArray = new long[blocks.size()];
        for (UnsignedLong block: blocks) blockArray[count++] = block.longValue();
        CryptoLibraryDirect.cwmAnnounceGetBlocksSuccess(thisPtr, callbackState.getPointer(), blockArray.length, blockArray);
    }

    public void announceGetBlocksFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetBlocksFailure(thisPtr, callbackState.getPointer());
    }

    public void announceGetTokensItem(BRCryptoCWMClientCallbackState callbackState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTokensItem(thisPtr, callbackState.getPointer(), address, symbol, name, description,
                decimals.intValue(), gasLimit, gasPrice);
    }

    public void announceGetTokensComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetTokensComplete(thisPtr, callbackState.getPointer(), success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    public void announceGetNonceSuccess(BRCryptoCWMClientCallbackState callbackState, String address, String nonce) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetNonceSuccess(thisPtr, callbackState.getPointer(), address, nonce);
    }

    public void announceGetNonceFailure(BRCryptoCWMClientCallbackState callbackState) {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cwmAnnounceGetNonceFailure(thisPtr, callbackState.getPointer());
    }

    public BRCryptoWalletManager take() {
        Pointer thisPtr = this.getPointer();

        return new BRCryptoWalletManager(CryptoLibraryDirect.cryptoWalletManagerTake(thisPtr));
    }

    public void give() {
        Pointer thisPtr = this.getPointer();

        CryptoLibraryDirect.cryptoWalletManagerGive(thisPtr);
    }
}
