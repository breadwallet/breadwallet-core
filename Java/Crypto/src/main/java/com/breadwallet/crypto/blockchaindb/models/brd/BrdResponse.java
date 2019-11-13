package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import static com.google.common.base.Preconditions.checkNotNull;

@JsonIgnoreProperties(value = {
        "jsonrpc",
        "id",
        "message",
        "status",
})
public class BrdResponse<T> {
    @JsonCreator
    public static <T> BrdResponse<T> create(@JsonProperty("result") T result) {
        return new BrdResponse<>(
                checkNotNull(result)
        );
    }

    private final T result;

    private BrdResponse(T result) {
        this.result = result;
    }

    public T getResult() {
        return result;
    }
}
