package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Wallet;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONObject;

public class WalletApi {

    private final BdbApiClient jsonClient;

    public WalletApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }


    public void getOrCreateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        getWallet(wallet.getId(), new BlockchainCompletionHandler<Wallet>() {
            @Override
            public void handleData(Wallet data) {
                handler.handleData(data);
            }

            @Override
            public void handleError(QueryError error) {
                createWallet(wallet, handler);
            }
        });
    }

    public void createWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.sendPost("wallets", ImmutableMultimap.of(), Wallet.asJson(wallet), Wallet::asWallet, handler);
    }

    public void getWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.sendGetWithId("wallets", id, ImmutableMultimap.of(), Wallet::asWallet, handler);
    }

    public void updateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.sendPutWithId("wallets", wallet.getId(), ImmutableMultimap.of(), Wallet.asJson(wallet),
                Wallet::asWallet, handler);
    }

    public void deleteWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.sendDeleteWithId("wallets", id, ImmutableMultimap.of(), Wallet::asWallet, handler);
    }
}
