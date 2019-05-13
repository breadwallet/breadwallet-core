package com.breadwallet.crypto.blockchaindb;

import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;

public interface BlockchainDataTask {
    void execute(OkHttpClient client, Request request, Callback callback);
}
