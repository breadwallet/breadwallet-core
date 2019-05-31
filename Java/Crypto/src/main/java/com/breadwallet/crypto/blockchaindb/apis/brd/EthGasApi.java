package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.json.JSONObject;

public class EthGasApi {

    private final BrdApiClient client;

    public EthGasApi(BrdApiClient client) {
        this.client = client;
    }

    public void getGasPriceAsEth(String networkName, int rid, BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_gasPrice",
                "params", ImmutableList.of(),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("0xffc0"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }

    public void getGasEstimateAsEth(String networkName, String from, String to, String amount, String data, int rid,
                                    BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_estimateGas",
                "params", ImmutableList.of(ImmutableMap.of("from", from, "to", to, "amount", amount, "data", data)),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, new BlockchainCompletionHandler<Optional<String>>() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO(discuss): Do we want default values?
                handler.handleData(result.or("92000"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
