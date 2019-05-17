package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

import org.json.JSONArray;

/* package */
interface ArrayCompletionHandler {
    void handleData(JSONArray json, boolean more);

    void handleError(QueryError error);
}
