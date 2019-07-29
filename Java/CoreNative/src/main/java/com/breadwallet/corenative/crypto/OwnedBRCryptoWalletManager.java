/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;
import java.util.Objects;

/* package */
class OwnedBRCryptoWalletManager implements CoreBRCryptoWalletManager {

    private final BRCryptoWalletManager core;

    /* package */
    OwnedBRCryptoWalletManager(BRCryptoWalletManager core) {
        this.core = core;
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        if (null != core) {
            CryptoLibrary.INSTANCE.cryptoWalletManagerGive(core);
        }
    }

    @Override
    public CoreBRCryptoAccount getAccount() {
        return core.getAccount();
    }

    @Override
    public CoreBRCryptoNetwork getNetwork() {
        return core.getNetwork();
    }

    @Override
    public CoreBRCryptoWallet getWallet() {
        return core.getWallet();
    }

    @Override
    public List<CoreBRCryptoWallet> getWallets() {
        return core.getWallets();
    }

    @Override
    public boolean containsWallet(CoreBRCryptoWallet wallet) {
        return core.containsWallet(wallet);
    }

    @Override
    public int getMode() {
        return core.getMode();
    }

    @Override
    public String getPath() {
        return core.getPath();
    }

    @Override
    public int getState() {
        return core.getState();
    }

    @Override
    public int getAddressScheme() {
        return core.getAddressScheme();
    }

    @Override
    public void setAddressScheme(int scheme) {
        core.setAddressScheme(scheme);
    }

    @Override
    public void connect() {
        core.connect();
    }

    @Override
    public void disconnect() {
        core.disconnect();
    }

    @Override
    public void sync() {
        core.sync();
    }

    @Override
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, byte[] phraseUtf8) {
        core.submit(wallet, transfer, phraseUtf8);
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, UnsignedLong blockchainHeight) {
        core.announceGetBlockNumberSuccess(callbackState, blockchainHeight);
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, String blockNumber) {
        core.announceGetBlockNumberSuccess(callbackState, blockNumber);
    }

    @Override
    public void announceGetBlockNumberFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetBlockNumberFailure(callbackState);
    }

    @Override
    public void announceGetTransactionsItemBtc(BRCryptoCWMClientCallbackState callbackState, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        core.announceGetTransactionsItemBtc(callbackState, transaction, timestamp, blockHeight);
    }

    @Override
    public void announceGetTransactionsItemEth(BRCryptoCWMClientCallbackState callbackState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        core.announceGetTransactionsItemEth(callbackState,hash, sourceAddr, targetAddr, contractAddr, amount, gasLimit,
                gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations, blockTransacionIndex,
                blockTimestamp, isError);
    }

    @Override
    public void announceGetTransactionsItemGen(BRCryptoCWMClientCallbackState callbackState, byte[] transaction,
                                               UnsignedLong timestamp, UnsignedLong blockHeight) {
        core.announceGetTransactionsItemGen(callbackState, transaction, timestamp, blockHeight);
    }

    @Override
    public void announceGetTransactionsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        core.announceGetTransactionsComplete(callbackState, success);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState) {
        core.announceSubmitTransferSuccess(callbackState);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState, String hash) {
        core.announceSubmitTransferSuccess(callbackState, hash);
    }

    @Override
    public void announceSubmitTransferFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceSubmitTransferFailure(callbackState);
    }

    @Override
    public void announceGetBalanceSuccess(BRCryptoCWMClientCallbackState callbackState, String balance) {
        core.announceGetBalanceSuccess(callbackState, balance);
    }

    @Override
    public void announceGetBalanceFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetBalanceFailure(callbackState);
    }

    @Override
    public void announceGetGasPriceSuccess(BRCryptoCWMClientCallbackState callbackState, String gasPrice) {
        core.announceGetGasPriceSuccess(callbackState, gasPrice);
    }

    @Override
    public void announceGetGasPriceFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetGasPriceFailure(callbackState);
    }

    @Override
    public void announceGetGasEstimateSuccess(BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice) {
        core.announceGetGasEstimateSuccess(callbackState, gasEstimate, gasPrice);
    }

    @Override
    public void announceGetGasEstimateFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetGasEstimateFailure(callbackState);
    }

    @Override
    public void announceGetLogsItem(BRCryptoCWMClientCallbackState callbackState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        core.announceGetLogsItem(callbackState, hash, contract, topics, data, gasPrice, gasUsed, logIndex, blockNumber,
                blockTransactionIndex, blockTimestamp);
    }

    @Override
    public void announceGetLogsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        core.announceGetLogsComplete(callbackState, success);
    }

    @Override
    public void announceGetBlocksSuccess(BRCryptoCWMClientCallbackState callbackState, List<UnsignedLong> blocks) {
        core.announceGetBlocksSuccess(callbackState, blocks);
    }

    @Override
    public void announceGetBlocksFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetBlocksFailure(callbackState);
    }

    @Override
    public void announceGetTokensItem(BRCryptoCWMClientCallbackState callbackState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        core.announceGetTokensItem(callbackState, address, symbol, name, description, decimals, gasLimit, gasPrice);
    }

    @Override
    public void announceGetTokensComplete(BRCryptoCWMClientCallbackState callbackState, boolean success) {
        core.announceGetTokensComplete(callbackState, success);
    }

    @Override
    public void announceGetNonceSuccess(BRCryptoCWMClientCallbackState callbackState, String address, String nonce) {
        core.announceGetNonceSuccess(callbackState, address, nonce);
    }

    @Override
    public void announceGetNonceFailure(BRCryptoCWMClientCallbackState callbackState) {
        core.announceGetNonceFailure(callbackState);
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (object instanceof OwnedBRCryptoWalletManager) {
            OwnedBRCryptoWalletManager that = (OwnedBRCryptoWalletManager) object;
            return core.equals(that.core);
        }

        if (object instanceof BRCryptoWalletManager) {
            BRCryptoWalletManager that = (BRCryptoWalletManager) object;
            return core.equals(that);
        }

        return false;
    }

    @Override
    public int hashCode() {
        return Objects.hash(core);
    }
}
