/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb;

import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;

public interface DataTask {
    void execute(OkHttpClient client, Request request, Callback callback);
}
