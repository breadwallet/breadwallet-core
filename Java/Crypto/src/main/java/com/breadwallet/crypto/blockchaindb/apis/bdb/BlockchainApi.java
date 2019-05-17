package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

import static com.google.common.base.Preconditions.checkArgument;

public class BlockchainApi {

    private final BdbApiClient jsonClient;

    public BlockchainApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getBlockchains(boolean ismainnet, BlockchainCompletionHandler<List<Blockchain>> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("testnet", Boolean.valueOf(!ismainnet).toString());
        jsonClient.sendGetRequest("blockchains", params, new ArrayCompletionHandler() {
            @Override
            public void handleData(JSONArray json, boolean more) {
                checkArgument(!more);
                Optional<List<Blockchain>> blockchains = Blockchain.asBlockchains(json);
                if (blockchains.isPresent()) {
                    handler.handleData(blockchains.get());
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

    public void getBlockchain(String id, BlockchainCompletionHandler<Blockchain> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("blockchains/%s", id);
        jsonClient.sendGetRequest(path, ImmutableListMultimap.of(), new ObjectCompletionHandler() {
            @Override
            public void handleData(JSONObject json, boolean more) {
                checkArgument(!more);
                Optional<Blockchain> blockchain = Blockchain.asBlockchain(json);
                if (blockchain.isPresent()) {
                    handler.handleData(blockchain.get());
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
