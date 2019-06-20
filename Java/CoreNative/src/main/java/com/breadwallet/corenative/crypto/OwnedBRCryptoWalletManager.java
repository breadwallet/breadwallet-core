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
    public CoreBRCryptoWallet getWallet() {
        return core.getWallet();
    }

    @Override
    public UnsignedLong getWalletsCount() {
        return core.getWalletsCount();
    }

    @Override
    public CoreBRCryptoWallet getWallet(UnsignedLong index) {
        return core.getWallet(index);
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
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey) {
        core.submit(wallet, transfer, paperKey);
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, UnsignedLong blockchainHeight) {
        core.announceGetBlockNumberSuccess(completetionState, blockchainHeight);
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, String blockNumber) {
        core.announceGetBlockNumberSuccess(completetionState, blockNumber);
    }

    @Override
    public void announceGetBlockNumberFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetBlockNumberFailure(completetionState);
    }

    @Override
    public void announceGetTransactionsItemBtc(BRCryptoCWMCompletionState completetionState, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        core.announceGetTransactionsItemBtc(completetionState, transaction, timestamp, blockHeight);
    }

    @Override
    public void announceGetTransactionsItemEth(BRCryptoCWMCompletionState completetionState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        core.announceGetTransactionsItemEth(completetionState,hash, sourceAddr, targetAddr, contractAddr, amount, gasLimit,
                gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations, blockTransacionIndex,
                blockTimestamp, isError);
    }

    @Override
    public void announceGetTransactionsComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        core.announceGetTransactionsComplete(completetionState, success);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState) {
        core.announceSubmitTransferSuccess(completetionState);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState, String hash) {
        core.announceSubmitTransferSuccess(completetionState, hash);
    }

    @Override
    public void announceSubmitTransferFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceSubmitTransferFailure(completetionState);
    }

    @Override
    public void announceGetBalanceSuccess(BRCryptoCWMCompletionState completetionState, String balance) {
        core.announceGetBalanceSuccess(completetionState, balance);
    }

    @Override
    public void announceGetBalanceFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetBalanceFailure(completetionState);
    }

    @Override
    public void announceGetGasPriceSuccess(BRCryptoCWMCompletionState completetionState, String gasPrice) {
        core.announceGetGasPriceSuccess(completetionState, gasPrice);
    }

    @Override
    public void announceGetGasPriceFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetGasPriceFailure(completetionState);
    }

    @Override
    public void announceGetGasEstimateSuccess(BRCryptoCWMCompletionState completetionState, String gasEstimate) {
        core.announceGetGasEstimateSuccess(completetionState, gasEstimate);
    }

    @Override
    public void announceGetGasEstimateFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetGasEstimateFailure(completetionState);
    }

    @Override
    public void announceGetLogsItem(BRCryptoCWMCompletionState completetionState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        core.announceGetLogsItem(completetionState, hash, contract, topics, data, gasPrice, gasUsed, logIndex, blockNumber,
                blockTransactionIndex, blockTimestamp);
    }

    @Override
    public void announceGetLogsComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        core.announceGetLogsComplete(completetionState, success);
    }

    @Override
    public void announceGetBlocksSuccess(BRCryptoCWMCompletionState completetionState, List<UnsignedLong> blocks) {
        core.announceGetBlocksSuccess(completetionState, blocks);
    }

    @Override
    public void announceGetBlocksFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetBlocksFailure(completetionState);
    }

    @Override
    public void announceGetTokensItem(BRCryptoCWMCompletionState completetionState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        core.announceGetTokensItem(completetionState, address, symbol, name, description, decimals, gasLimit, gasPrice);
    }

    @Override
    public void announceGetTokensComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        core.announceGetTokensComplete(completetionState, success);
    }

    @Override
    public void announceGetNonceSuccess(BRCryptoCWMCompletionState completetionState, String address, String nonce) {
        core.announceGetNonceSuccess(completetionState, address, nonce);
    }

    @Override
    public void announceGetNonceFailure(BRCryptoCWMCompletionState completetionState) {
        core.announceGetNonceFailure(completetionState);
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
