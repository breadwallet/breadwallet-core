/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/22/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.cryptodemo;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class ApplicationExecutors {

    private static final ExecutorService uiExecutor = Executors.newSingleThreadExecutor();
    private static final ExecutorService blockingExecutor = Executors.newCachedThreadPool();

    public static void runOnUiExecutor(Runnable runnable) {
        uiExecutor.submit(runnable);
    }

    public static void runOnBlockingExecutor(Runnable runnable) {
        blockingExecutor.submit(runnable);
    }

    private ApplicationExecutors() {}
}
