package com.breadwallet.crypto.blockchaindb.apis.brd;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthLog;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

public class EthLogApi {

    private final BrdApiClient client;

    public EthLogApi(BrdApiClient client) {
        this.client = client;
    }

    public void getLogsAsEth(String networkName, @Nullable String contract, String address, String event,
                             long begBlockNumber, long endBlockNumber, int rid,
                             BlockchainCompletionHandler<List<EthLog>> handler) {
        JSONObject json = new JSONObject(ImmutableMap.of(
                "id", Integer.valueOf(rid)
        ));

        ImmutableListMultimap.Builder<String, String> paramsBuilders = ImmutableListMultimap.builder();
        paramsBuilders.put("module", "logs");
        paramsBuilders.put("action", "getLogs");
        paramsBuilders.put("fromBlock", String.valueOf(begBlockNumber));
        paramsBuilders.put("toBlock", String.valueOf(endBlockNumber));
        paramsBuilders.put("topic0", event);
        paramsBuilders.put("topic1", address);
        paramsBuilders.put("topic_1_2_opr", "or");
        paramsBuilders.put("topic2", address);

        if (null != contract) {
            paramsBuilders.put("address", contract);
        }

        client.makeRequestQuery(networkName, paramsBuilders.build(), json, new ArrayCompletionHandler() {
            @Override
            public void handleData(JSONArray json) {
                Optional<List<EthLog>> logs = EthLog.asLogs(json, rid);
                if (logs.isPresent()) {
                    handler.handleData(logs.get());
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
}
