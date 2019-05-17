package com.breadwallet.crypto.blockchaindb.apis.brd;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;
import com.google.common.base.Optional;

import org.json.JSONObject;

/* package */
interface StringCompletionHandler {
    void handleData(Optional<String> result);

    void handleError(QueryError error);
}
