package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.models.bdb.Blockchain;
import com.google.common.collect.ImmutableListMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.Multimap;

import java.util.List;

public class BlockchainApi {

    private final BdbApiClient jsonClient;

    public BlockchainApi(BdbApiClient jsonClient) {
        this.jsonClient = jsonClient;
    }

    public void getBlockchains(boolean isMainnet, BlockchainCompletionHandler<List<Blockchain>> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("testnet", Boolean.valueOf(!isMainnet).toString());
        jsonClient.sendGetForArray("blockchains", params, Blockchain::asBlockchains, handler);
    }

    public void getBlockchain(String id, BlockchainCompletionHandler<Blockchain> handler) {
        jsonClient.sendGetWithId("blockchains", id, ImmutableMultimap.of(), Blockchain::asBlockchain, handler);
    }
}
