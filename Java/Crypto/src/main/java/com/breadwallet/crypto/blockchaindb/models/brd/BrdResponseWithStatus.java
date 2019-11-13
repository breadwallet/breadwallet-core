package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import static com.google.common.base.Preconditions.checkNotNull;

@JsonIgnoreProperties(value = {
        "jsonrpc",
        "id",
})
public class BrdResponseWithStatus<T> {
    @JsonCreator
    public static <T> BrdResponseWithStatus<T> create(@JsonProperty("status") String status,
                                                      @JsonProperty("message") String message,
                                                      @JsonProperty("result") T result) {
        checkNotNull(status);
        checkNotNull(message);
        return new BrdResponseWithStatus<>(
                checkNotNull(result)
        );
    }

    private final T result;

    private BrdResponseWithStatus(T result) {
        this.result = result;
    }

    public T getResult() {
        return result;
    }
}
