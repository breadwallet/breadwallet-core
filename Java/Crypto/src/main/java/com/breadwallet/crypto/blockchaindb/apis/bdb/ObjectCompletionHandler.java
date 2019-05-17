package com.breadwallet.crypto.blockchaindb.apis.bdb;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

import org.json.JSONObject;

/* package */
interface ObjectCompletionHandler {
    void handleData(JSONObject json, boolean more);

    void handleError(QueryError error);
}
