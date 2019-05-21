package com.breadwallet.crypto.blockchaindb.apis;

import com.google.common.base.Optional;

import org.json.JSONArray;

public interface ArrayResponseParser<T> {
    Optional<T> parse(JSONArray json);
}
