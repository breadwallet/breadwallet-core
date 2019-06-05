/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.CompletionHandler;
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

    public void getBlockchains(boolean isMainnet, CompletionHandler<List<Blockchain>> handler) {
        Multimap<String, String> params = ImmutableListMultimap.of("testnet", Boolean.valueOf(!isMainnet).toString());
        jsonClient.sendGetForArray("blockchains", params, Blockchain::asBlockchains, handler);
    }

    public void getBlockchain(String id, CompletionHandler<Blockchain> handler) {
        jsonClient.sendGetWithId("blockchains", id, ImmutableMultimap.of(), Blockchain::asBlockchain, handler);
    }
}
