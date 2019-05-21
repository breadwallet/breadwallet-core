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
        jsonClient.sendGetRequest("blockchains", params,  Blockchain::asBlockchains, handler);
    }

    public void getBlockchain(String id, BlockchainCompletionHandler<Blockchain> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("blockchains/%s", id);
        jsonClient.sendGetRequest(path, ImmutableListMultimap.of(), Blockchain::asBlockchain, handler);
    }
}
