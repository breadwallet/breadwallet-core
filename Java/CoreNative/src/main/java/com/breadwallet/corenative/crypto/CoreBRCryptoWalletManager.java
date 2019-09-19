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
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;

public interface CoreBRCryptoWalletManager {

    static CoreBRCryptoWalletManager create(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client,
                                            CoreBRCryptoAccount account, CoreBRCryptoNetwork network, int mode,
                                            int scheme, String path) {
        return new OwnedBRCryptoWalletManager(CryptoLibrary.INSTANCE.cryptoWalletManagerCreate(
                listener, client, account.asBRCryptoAccount(), network.asBRCryptoNetwork(), mode, scheme, path));
    }

    static CoreBRCryptoWalletManager createOwned(BRCryptoWalletManager manager) {
        return new OwnedBRCryptoWalletManager(manager);
    }

    CoreBRCryptoAccount getAccount();

    CoreBRCryptoNetwork getNetwork();

    CoreBRCryptoWallet getWallet();

    List<CoreBRCryptoWallet> getWallets();

    boolean containsWallet(CoreBRCryptoWallet wallet);

    Optional<CoreBRCryptoWallet> registerWallet(CoreBRCryptoCurrency currency);

    void setNetworkReachable(boolean isNetworkReachable);

    int getMode();

    void setMode(int mode);

    String getPath();

    BRCryptoWalletManagerState getState();

    int getAddressScheme();

    void setAddressScheme(int scheme);

    void connect();

    void disconnect();

    void sync();

    void syncToDepth(BRSyncDepth depth);

    void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, byte[] phraseUtf8);

    void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, BRCryptoKey key);

    void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, UnsignedLong blockchainHeight);

    void announceGetBlockNumberSuccess(BRCryptoCWMClientCallbackState callbackState, String blockNumber);

    void announceGetBlockNumberFailure(BRCryptoCWMClientCallbackState callbackState);

    void announceGetTransactionsItemBtc(BRCryptoCWMClientCallbackState callbackState, byte[] transaction, UnsignedLong timestamp,
                                        UnsignedLong blockHeight);

    void announceGetTransactionsItemEth(BRCryptoCWMClientCallbackState callbackState, String hash, String sourceAddr,
                                        String targetAddr, String contractAddr, String amount, String gasLimit,
                                        String gasPrice, String data,
                                        String nonce, String gasUsed, String blockNumber, String blockHash,
                                        String blockConfirmations, String blockTransacionIndex, String blockTimestamp,
                                        String isError);

    void announceGetTransactionsItemGen(BRCryptoCWMClientCallbackState callbackState, byte[] transaction, UnsignedLong timestamp,
                                        UnsignedLong blockHeight);

    void announceGetTransactionsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success);

    void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState);

    void announceSubmitTransferSuccess(BRCryptoCWMClientCallbackState callbackState, String hash);

    void announceSubmitTransferFailure(BRCryptoCWMClientCallbackState callbackState);

    void announceGetBalanceSuccess(BRCryptoCWMClientCallbackState callbackState, String balance);

    void announceGetBalanceFailure(BRCryptoCWMClientCallbackState callbackState);

    void announceGetGasPriceSuccess(BRCryptoCWMClientCallbackState callbackState, String gasPrice);

    void announceGetGasPriceFailure(BRCryptoCWMClientCallbackState callbackState);

    void announceGetGasEstimateSuccess(BRCryptoCWMClientCallbackState callbackState, String gasEstimate, String gasPrice);

    void announceGetGasEstimateFailure(BRCryptoCWMClientCallbackState callbackState, int status);

    void announceGetLogsItem(BRCryptoCWMClientCallbackState callbackState, String hash, String contract, List<String> topics,
                             String data, String gasPrice, String gasUsed, String logIndex, String blockNumber,
                             String blockTransactionIndex, String blockTimestamp);

    void announceGetLogsComplete(BRCryptoCWMClientCallbackState callbackState, boolean success);

    void announceGetBlocksSuccess(BRCryptoCWMClientCallbackState callbackState, List<UnsignedLong> blocks);

    void announceGetBlocksFailure(BRCryptoCWMClientCallbackState callbackState);

    void announceGetTokensItem(BRCryptoCWMClientCallbackState callbackState, String address, String symbol, String name,
                               String description, UnsignedInteger decimals, String gasLimit, String gasPrice);

    void announceGetTokensComplete(BRCryptoCWMClientCallbackState callbackState, boolean success);

    void announceGetNonceSuccess(BRCryptoCWMClientCallbackState callbackState, String address, String nonce);

    void announceGetNonceFailure(BRCryptoCWMClientCallbackState callbackState);
}
