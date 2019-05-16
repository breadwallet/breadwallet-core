package com.breadwallet.crypto.blockchaindb.apis;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.Wallet;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONObject;

public class WalletJsonApi {

    private final BdbApiClient jsonClient;

    public WalletJsonApi(BdbApiClient jsonClient) {
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

    public void getWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", id);
        makeWalletRequest(path, "GET", handler);
    }

    public void createWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        makeWalletRequest(wallet, "wallets", "POST", handler);
    }

    public void updateWallet(Wallet wallet, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", wallet.getId());
        makeWalletRequest(wallet, path, "PUT", handler);
    }

    public void deleteWallet(String id, BlockchainCompletionHandler<Wallet> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("wallets/%s", id);
        makeWalletRequest(path, "DELETE", handler);
    }

    private void makeWalletRequest(Wallet wallet, String path, String httpMethod, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.makeRequest(path, ImmutableMultimap.of(), Wallet.asJson(wallet), httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Wallet> optionalWallet = Wallet.asWallet(data);
                if (optionalWallet.isPresent()) {
                    handler.handleData(optionalWallet.get());
                } else {
                    handler.handleError(new QueryModelError("Missed wallet"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    private void makeWalletRequest(String path, String httpMethod, BlockchainCompletionHandler<Wallet> handler) {
        jsonClient.makeRequest(path, ImmutableMultimap.of(), null, httpMethod, new BlockchainCompletionHandler<JSONObject>() {
            @Override
            public void handleData(JSONObject data) {
                Optional<Wallet> optionalWallet = Wallet.asWallet(data);
                if (optionalWallet.isPresent()) {
                    handler.handleData(optionalWallet.get());
                } else {
                    handler.handleError(new QueryModelError("Missed wallet"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
