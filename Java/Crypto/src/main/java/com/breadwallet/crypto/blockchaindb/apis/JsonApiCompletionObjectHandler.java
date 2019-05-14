package com.breadwallet.crypto.blockchaindb.apis;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

import org.json.JSONObject;

public interface JsonApiCompletionObjectHandler {
    void handleData(JSONObject json, boolean more);

    void handleError(QueryError error);
}
