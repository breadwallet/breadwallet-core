package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.json.JSONObject;

public class EthBlockApi {

    private final BrdApiClient client;

    public EthBlockApi(BrdApiClient client) {
        this.client = client;
    }

    public void getBlockNumberAsEth(String networkName, int rid, BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_blockNumber",
                "params", ImmutableList.of(),
                "id", rid
        ));

        client.makeRequestJson(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> data) {
                handler.handleData(data.or("0xffc0"));
            }

            @Override
            public void handleError(QueryError error) {

            }
        });
    }
}
