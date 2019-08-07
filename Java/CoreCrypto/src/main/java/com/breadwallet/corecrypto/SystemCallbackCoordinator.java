/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.sun.jna.Pointer;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

/* package */
final class SystemCallbackCoordinator {

    private static final AtomicInteger HANDLER_IDS = new AtomicInteger(0);

    private final ScheduledExecutorService executor;

    private final Map<Pointer, CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError>> handlers;

    /* package */
    SystemCallbackCoordinator(ScheduledExecutorService executor) {
        this.executor = executor;
        this.handlers = new ConcurrentHashMap<>();
    }

    // Operation callbacks

    /* package */
    Pointer registerFeeBasisEstimateHandler(CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler) {
        Pointer cookie = Pointer.createConstant(HANDLER_IDS.incrementAndGet());
        handlers.put(cookie, handler);
        return cookie;
    }

    /* package */
    void completeFeeBasisEstimateHandlerWithSuccess(Pointer cookie, TransferFeeBasis feeBasis) {
        CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler = handlers.remove(cookie);
        if (null != handler) {
            executor.submit(() -> handler.handleData(feeBasis));
        }
    }

    /* package */
    void completeFeeBasisEstimateHandlerWithError(Pointer cookie, FeeEstimationError error) {
        CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> handler = handlers.remove(cookie);
        if (null != handler) {
            executor.submit(() -> handler.handleError(error));
        }
    }
}
