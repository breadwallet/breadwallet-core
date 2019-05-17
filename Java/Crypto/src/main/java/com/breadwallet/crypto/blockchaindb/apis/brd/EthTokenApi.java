package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.google.common.base.Optional;

import org.json.JSONArray;

import java.util.List;

public class EthTokenApi {

    private final BrdApiClient client;

    public EthTokenApi(BrdApiClient client) {
        this.client = client;
    }

    public void getTokensAsEth(int rid, BlockchainCompletionHandler<List<EthToken>> handler) {
        client.makeRequestToken(new ArrayCompletionHandler() {
            @Override
            public void handleData(JSONArray json) {
                Optional<List<EthToken>> logs = EthToken.asTokens(json, rid);
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
