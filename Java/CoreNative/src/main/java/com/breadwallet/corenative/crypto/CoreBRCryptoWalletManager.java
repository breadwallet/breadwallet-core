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

public interface CoreBRCryptoWalletManager {

    static CoreBRCryptoWalletManager create(BRCryptoWalletManager manager) {
        return new OwnedBRCryptoWalletManager(manager);
    }

    static CoreBRCryptoWalletManager create(BRCryptoCWMListener.ByValue listener, BRCryptoCWMClient.ByValue client,
                                            CoreBRCryptoAccount account, CoreBRCryptoNetwork network, int mode,
                                            String path) {
        return new OwnedBRCryptoWalletManager(CryptoLibrary.INSTANCE.cryptoWalletManagerCreate(
                listener, client, account.asBRCryptoAccount(), network.asBRCryptoNetwork(), mode, path));
    }

    CoreBRCryptoWallet getWallet();

    UnsignedLong getWalletsCount();

    CoreBRCryptoWallet getWallet(UnsignedLong index);

    boolean containsWallet(CoreBRCryptoWallet wallet);

    int getMode();

    String getPath();

    int getState();

    void connect();

    void disconnect();

    void sync();

    void submit(CoreBRCryptoWallet wallet, CoreBRCryptoTransfer transfer, String paperKey);

    void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, UnsignedLong blockchainHeight);

    void announceGetBlockNumberSuccess(BRCryptoCWMCompletionState completetionState, String blockNumber);

    void announceGetBlockNumberFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetTransactionsItemBtc(BRCryptoCWMCompletionState completetionState, byte[] transaction, UnsignedLong timestamp,
                                        UnsignedLong blockHeight);

    void announceGetTransactionsItemEth(BRCryptoCWMCompletionState completetionState, String hash, String sourceAddr,
                                        String targetAddr, String contractAddr, String amount, String gasLimit,
                                        String gasPrice, String data,
                                        String nonce, String gasUsed, String blockNumber, String blockHash,
                                        String blockConfirmations, String blockTransacionIndex, String blockTimestamp,
                                        String isError);

    void announceGetTransactionsComplete(BRCryptoCWMCompletionState completetionState, boolean success);

    void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState);

    void announceSubmitTransferSuccess(BRCryptoCWMCompletionState completetionState, String hash);

    void announceSubmitTransferFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetBalanceSuccess(BRCryptoCWMCompletionState completetionState, String balance);

    void announceGetBalanceFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetGasPriceSuccess(BRCryptoCWMCompletionState completetionState, String gasPrice);

    void announceGetGasPriceFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetGasEstimateSuccess(BRCryptoCWMCompletionState completetionState, String gasEstimate);

    void announceGetGasEstimateFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetLogsItem(BRCryptoCWMCompletionState completetionState, String hash, String contract, List<String> topics,
                             String data, String gasPrice, String gasUsed, String logIndex, String blockNumber,
                             String blockTransactionIndex, String blockTimestamp);

    void announceGetLogsComplete(BRCryptoCWMCompletionState completetionState, boolean success);

    void announceGetBlocksSuccess(BRCryptoCWMCompletionState completetionState, List<UnsignedLong> blocks);

    void announceGetBlocksFailure(BRCryptoCWMCompletionState completetionState);

    void announceGetTokensItem(BRCryptoCWMCompletionState completetionState, String address, String symbol, String name,
                               String description, UnsignedInteger decimals, String gasLimit, String gasPrice);

    void announceGetTokensComplete(BRCryptoCWMCompletionState completetionState, boolean success);

    void announceGetNonceSuccess(BRCryptoCWMCompletionState completetionState, String address, String nonce);

    void announceGetNonceFailure(BRCryptoCWMCompletionState completetionState);
}
