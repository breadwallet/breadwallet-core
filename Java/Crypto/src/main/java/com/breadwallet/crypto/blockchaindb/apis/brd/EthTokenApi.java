package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.BlockchainCompletionHandler;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;

import java.util.List;

public class EthTokenApi {

    private final BrdApiClient client;

    public EthTokenApi(BrdApiClient client) {
        this.client = client;
    }

    public void getTokensAsEth(int rid, BlockchainCompletionHandler<List<EthToken>> handler) {
        client.makeRequestToken(EthToken::asTokens, handler);
    }
}
