package com.breadwallet.crypto.blockchaindb.errors;

public class QueryNoEntityError extends QueryError {

    private final String id;

    public QueryNoEntityError(String id) {
        super("No entity for id " + id);
        this.id = id;
    }

    public String getUrl() {
        return id;
    }
}
