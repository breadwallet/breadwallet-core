/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import java.util.Map;

public class EthGasApi {

    private final BrdApiClient client;

    public EthGasApi(BrdApiClient client) {
        this.client = client;
    }

    public void getGasPriceAsEth(String networkName,
                                 int rid,
                                 CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_gasPrice",
                "params", ImmutableList.of(),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getGasEstimateAsEth(String networkName,
                                    String from,
                                    String to,
                                    String amount,
                                    String data,
                                    int rid,
                                    CompletionHandler<String, QueryError> handler) {
        ImmutableMap.Builder<String, String> paramsBuilder = new ImmutableMap.Builder<>();
        paramsBuilder.put("from", from);
        paramsBuilder.put("to", to);
        if (!amount.equals("0x")) paramsBuilder.put("value", amount);
        if (!data.equals("0x")) paramsBuilder.put("data", data);

        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_estimateGas",
                "params", ImmutableList.of(paramsBuilder.build()),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, handler);
    }
}
