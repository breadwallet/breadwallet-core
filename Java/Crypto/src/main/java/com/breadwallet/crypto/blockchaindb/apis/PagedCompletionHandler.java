package com.breadwallet.crypto.blockchaindb.apis;

public interface PagedCompletionHandler<T, E> {

    void handleData(T data, PageInfo info);
    void handleError(E error);
}
