package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONObject;

public class EthBalanceApi {

    private final BrdApiClient client;

    public EthBalanceApi(BrdApiClient client) {
        this.client = client;
    }

    public void getBalanceAsEth(String networkName, String address, int rid,
                                BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_getBalance",
                "params", ImmutableList.of(address, "latest"),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("200000000000000000"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getBalanceAsTok(String networkName, String address, String tokenAddress, int rid,
                                BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "id", rid
        ));

        ImmutableMultimap<String, String> params = ImmutableListMultimap.of(
                "module", "account",
                "action", "tokenbalance",
                "address", address,
                "contractaddress", tokenAddress
        );

        client.sendQueryRequest(networkName, params, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("0x1"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
