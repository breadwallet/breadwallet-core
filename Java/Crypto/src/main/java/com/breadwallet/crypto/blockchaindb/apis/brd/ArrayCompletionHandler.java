package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

import org.json.JSONArray;

/* package */
interface ArrayCompletionHandler {
    void handleData(JSONArray json);

    void handleError(QueryError error);
}
