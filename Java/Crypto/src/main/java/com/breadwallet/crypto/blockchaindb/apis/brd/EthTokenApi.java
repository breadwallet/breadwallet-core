/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.breadwallet.crypto.blockchaindb.models.brd.EthToken;
import com.breadwallet.crypto.utility.CompletionHandler;

import java.util.List;

public class EthTokenApi {

    private final BrdApiClient client;

    public EthTokenApi(BrdApiClient client) {
        this.client = client;
    }

    public void getTokensAsEth(int rid,
                               CompletionHandler<List<EthToken>, QueryError> handler) {
        client.sendTokenRequest(EthToken.class, handler);
    }
}
