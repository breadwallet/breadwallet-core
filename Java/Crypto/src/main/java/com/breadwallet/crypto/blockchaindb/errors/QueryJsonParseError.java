package com.breadwallet.crypto.blockchaindb.errors;

public class QueryJsonParseError extends QueryError {

    public QueryJsonParseError() {
        super();
    }

    public QueryJsonParseError(String message) {
        super(message);
    }
}
