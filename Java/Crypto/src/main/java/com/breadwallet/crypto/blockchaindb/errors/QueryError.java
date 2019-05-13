package com.breadwallet.crypto.blockchaindb.errors;

public abstract  class QueryError extends Exception {

    public QueryError() {
        super();
    }

    public QueryError(String message) {
        super(message);
    }
}
