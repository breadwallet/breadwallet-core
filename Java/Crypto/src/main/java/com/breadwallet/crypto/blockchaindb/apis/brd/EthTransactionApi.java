package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthTransaction;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableMultimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

public class EthTransactionApi {

    private final BrdApiClient client;

    public EthTransactionApi(BrdApiClient client) {
        this.client = client;
    }

    public void getTransactionsAsEth(String networkName, String address, long begBlockNumber, long endBlockNumber,
                                     int rid, BlockchainCompletionHandler<List<EthTransaction>> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "id", Integer.valueOf(rid),
                "account", address
        ));

        ImmutableMultimap<String, String> params = ImmutableListMultimap.of(
                "module", "account",
                "action", "txlist",
                "address", address,
                "startBlock", String.valueOf(begBlockNumber),
                "endBlock", String.valueOf(endBlockNumber)
        );

        client.makeRequestQuery(networkName, params, json, new ArrayCompletionHandler() {
            @Override
            public void handleData(JSONArray json) {
                Optional<List<EthTransaction>> transactions = EthTransaction.asTransactions(json, rid);
                if (transactions.isPresent()) {
                    handler.handleData(transactions.get());
                } else {
                    handler.handleError(new QueryModelError("Transform error"));
                }
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
    public void submitTransactionAsEth(String networkName, String transaction, int rid,
                                       BlockchainCompletionHandler<String> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "jsonrpc", "2.0",
                "method", "eth_sendRawTransaction",
                "params", ImmutableList.of(transaction),
                "id", Integer.valueOf(rid)
        ));

        client.makeRequestJson(networkName, json, new StringCompletionHandler() {
            @Override
            public void handleData(Optional<String> result) {
                // TODO: Do we want default values?
                handler.handleData(result.or("0x123abc456def"));
            }

            @Override
            public void handleError(QueryError error) {
                handler.handleError(error);
            }
        });
    }
}
