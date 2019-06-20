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
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.util.List;

public class BRCryptoWalletManager extends PointerType implements CoreBRCryptoWalletManager {

    public BRCryptoWalletManager(Pointer address) {
        super(address);
    }

    public BRCryptoWalletManager() {
        super();
    }

    @Override
    public CoreBRCryptoWallet getWallet() {
        return new OwnedBRCryptoWallet(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWallet(this));
    }

    @Override
    public UnsignedLong getWalletsCount() {
        return UnsignedLong.fromLongBits(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletsCount(this).longValue());
    }

    @Override
    public CoreBRCryptoWallet getWallet(UnsignedLong index) {
        return new OwnedBRCryptoWallet(CryptoLibrary.INSTANCE.cryptoWalletManagerGetWalletAtIndex(this, new SizeT(index.longValue())));
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
    public String getPath() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetPath(this).getString(0, "UTF-8");
    }

    @Override
    public int getState() {
        return CryptoLibrary.INSTANCE.cryptoWalletManagerGetState(this);
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
    public void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey) {
        CryptoLibrary.INSTANCE.cryptoWalletManagerSubmit(this, wallet.asBRCryptoWallet(), transfer.asBRCryptoTransfer(), paperKey);
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, UnsignedLong blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsInteger(this, completetionState, blockNumber.longValue());
    }

    @Override
    public void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, String blockNumber) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberSuccessAsString(this, completetionState, blockNumber);
    }

    @Override
    public void announceGetBlockNumberFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlockNumberFailure(this, completetionState);
    }

    @Override
    public void announceGetTransactionsItemBtc(BRCryptoCWMCompletionState completetionState, byte[] transaction, UnsignedLong timestamp,
                                               UnsignedLong blockHeight) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemBTC(this, completetionState, transaction, new SizeT(transaction.length),
                timestamp.longValue(), blockHeight.longValue());
    }

    @Override
    public void announceGetTransactionsItemEth(BRCryptoCWMCompletionState completetionState, String hash, String sourceAddr,
                                               String targetAddr, String contractAddr, String amount, String gasLimit,
                                               String gasPrice, String data, String nonce, String gasUsed,
                                               String blockNumber, String blockHash, String blockConfirmations,
                                               String blockTransacionIndex, String blockTimestamp, String isError) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsItemETH(this, completetionState, hash, sourceAddr, targetAddr, contractAddr,
                amount, gasLimit, gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations,
                blockTransacionIndex, blockTimestamp, isError);
    }

    @Override
    public void announceGetTransactionsComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTransactionsComplete(this, completetionState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccess(this, completetionState);
    }

    @Override
    public void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState, String hash) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferSuccessForHash(this, completetionState, hash);
    }

    @Override
    public void announceSubmitTransferFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceSubmitTransferFailure(this, completetionState);
    }

    @Override
    public void announceGetBalanceSuccess(BRCryptoCWMCompletionState completetionState, String balance) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceSuccess(this, completetionState, balance);
    }

    @Override
    public void announceGetBalanceFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBalanceFailure(this, completetionState);
    }

    @Override
    public void announceGetGasPriceSuccess(BRCryptoCWMCompletionState completetionState, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceSuccess(this, completetionState, gasPrice);
    }

    @Override
    public void announceGetGasPriceFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasPriceFailure(this, completetionState);
    }

    @Override
    public void announceGetGasEstimateSuccess(BRCryptoCWMCompletionState completetionState, String gasEstimate) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateSuccess(this, completetionState, gasEstimate);
    }

    @Override
    public void announceGetGasEstimateFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetGasEstimateFailure(this, completetionState);
    }

    @Override
    public void announceGetLogsItem(BRCryptoCWMCompletionState completetionState, String hash, String contract,
                                    List<String> topics, String data, String gasPrice, String gasUsed,
                                    String logIndex, String blockNumber, String blockTransactionIndex,
                                    String blockTimestamp) {
        StringArray topicsArray = new StringArray(topics.toArray(new String[0]), "UTF-8");
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsItem(this, completetionState, hash, contract, topics.size(),
                topicsArray, data, gasPrice, gasUsed, logIndex,
                blockNumber, blockTransactionIndex, blockTimestamp);
    }

    @Override
    public void announceGetLogsComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetLogsComplete(this, completetionState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceGetBlocksSuccess(BRCryptoCWMCompletionState completetionState, List<UnsignedLong> blocks) {
        int count = 0;
        long[] blockArray = new long[blocks.size()];
        for (UnsignedLong block: blocks) blockArray[count++] = block.longValue();
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksSuccess(this, completetionState, blockArray.length, blockArray);
    }

    @Override
    public void announceGetBlocksFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetBlocksFailure(this, completetionState);
    }

    @Override
    public void announceGetTokensItem(BRCryptoCWMCompletionState completetionState, String address, String symbol, String name,
                                      String description, UnsignedInteger decimals, String gasLimit, String gasPrice) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensItem(this, completetionState, address, symbol, name, description,
                decimals.intValue(), gasLimit, gasPrice);
    }

    @Override
    public void announceGetTokensComplete(BRCryptoCWMCompletionState completetionState, boolean success) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetTokensComplete(this, completetionState, success ? BRCryptoBoolean.CRYPTO_TRUE :
                BRCryptoBoolean.CRYPTO_FALSE);
    }

    @Override
    public void announceGetNonceSuccess(BRCryptoCWMCompletionState completetionState, String address, String nonce) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceSuccess(this, completetionState, address, nonce);
    }

    @Override
    public void announceGetNonceFailure(BRCryptoCWMCompletionState completetionState) {
        CryptoLibrary.INSTANCE.cwmAnnounceGetNonceFailure(this, completetionState);
    }
}
