package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.errors.QueryModelError;
import com.breadwallet.crypto.blockchaindb.models.bdb.Transfer;
import com.google.common.base.Optional;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.Multimap;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;

import static com.google.common.base.Preconditions.checkArgument;

public class TransferApi {

    private final BdbApiClient jsonClient;

    public TransferApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getTransfers(String id, List<String> addresses, BlockchainCompletionHandler<List<Transfer>> handler) {
        ImmutableListMultimap.Builder<String, String> paramBuilders = ImmutableListMultimap.builder();
        paramBuilders.put("blockchain_id", id);
        for (String address : addresses) paramBuilders.put("address", address);
        Multimap<String, String> params = paramBuilders.build();

        jsonClient.sendGetRequest("transfers", params, Transfer::asTransfers, handler);
    }

    public void getTransfer(String id, BlockchainCompletionHandler<Transfer> handler) {
        // TODO: I don't think we should be building it like this
        String path = String.format("transfers/%s", id);
        jsonClient.sendGetRequest(path, ImmutableListMultimap.of(), Transfer::asTransfer, handler);
    }

}
