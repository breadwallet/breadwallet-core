/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import org.json.JSONObject;

public class EthGasApi {

    private final BrdApiClient client;

    public EthGasApi(BrdApiClient client) {
        this.client = client;
    }

    public void getGasPriceAsEth(String networkName, int rid, CompletionHandler<String, QueryError> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_gasPrice",
                "params", ImmutableList.of(),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getGasEstimateAsEth(String networkName, String from, String to, String amount, String data, int rid,
                                    CompletionHandler<String, QueryError> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_estimateGas",
                "params", ImmutableList.of(ImmutableMap.of("from", from, "to", to, "amount", amount, "data", data)),
                "id", rid
        ));

        client.sendJsonRequest(networkName, json, handler);
    }
}
