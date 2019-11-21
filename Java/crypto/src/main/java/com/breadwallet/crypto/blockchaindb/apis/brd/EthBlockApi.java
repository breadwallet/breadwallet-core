/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryNoDataError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

import java.util.Map;

public class EthBlockApi {

    private final BrdApiClient client;

    public EthBlockApi(BrdApiClient client) {
        this.client = client;
    }

    public void getBlockNumberAsEth(String networkName,
                                    int rid,
                                    CompletionHandler<String, QueryError> handler) {
        Map json = ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_blockNumber",
                "params", ImmutableList.of(),
                "id", rid
        );

        client.sendJsonRequest(networkName, json, new CompletionHandler<String, QueryError>() {
            @Override
            public void handleData(String data) {
                // If we get a successful response, but the provided blocknumber is "0" then
                // that indicates that the JSON-RPC node is syncing.  Thus, if "0" transform
                // to a .failure
                if (!"0".equals(data) && !"0x".equals(data)) {
                    handler.handleData(data);
                } else {
                    handler.handleError(new QueryNoDataError());
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
