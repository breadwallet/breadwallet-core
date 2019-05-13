package com.breadwallet.crypto.blockchaindb;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

public interface BlockchainCompletionHandler<T> {
    void handleData(T data);
    void handleError(QueryError error);
}
