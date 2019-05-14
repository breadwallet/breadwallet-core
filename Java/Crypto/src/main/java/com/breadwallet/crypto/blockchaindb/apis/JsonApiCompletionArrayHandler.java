package com.breadwallet.crypto.blockchaindb.apis;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

import org.json.JSONArray;

public interface JsonApiCompletionArrayHandler {
    void handleData(JSONArray json, boolean more);

    void handleError(QueryError error);
}
