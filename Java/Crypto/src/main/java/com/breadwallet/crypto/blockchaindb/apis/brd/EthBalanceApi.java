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
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;

import java.util.Map;

public class EthBalanceApi {

    private final BrdApiClient client;

    public EthBalanceApi(BrdApiClient client) {
        this.client = client;
    }

    public void getBalanceAsEth(String networkName,
                                String address,
                                int rid,
                                CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_getBalance",
                "params", ImmutableList.of(address, "latest"),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, handler);
    }

    public void getBalanceAsTok(String networkName,
                                String address,
                                String tokenAddress,
                                int rid,
                                CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "id", rid
        );

        ImmutableMultimap<String, String> params = ImmutableListMultimap.of(
                "module", "account",
                "action", "tokenbalance",
                "address", address,
                "contractaddress", tokenAddress
        );

        client.sendQueryRequest(networkName, params, json, handler);
    }
}
