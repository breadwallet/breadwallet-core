package com.breadwallet.crypto.blockchaindb.apis;

import com.google.common.base.Optional;

import org.json.JSONObject;

public interface ObjectResponseParser<T> {
    Optional<T> parse(JSONObject json);
}
