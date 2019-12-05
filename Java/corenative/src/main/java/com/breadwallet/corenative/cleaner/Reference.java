/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/10/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.cleaner;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.Collections;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/* package */
final class Reference extends PhantomReference<Object> implements Runnable {

    /* package */
    static void create(ReferenceQueue<Object> queue, Object referent, Runnable runnable) {
        REFS.add(new Reference(queue, referent, runnable));
    }

    private static final Set<Reference> REFS = Collections.newSetFromMap(new ConcurrentHashMap<>());

    private final Runnable runnable;

    private Reference(ReferenceQueue<Object> queue, Object referent, Runnable runnable) {
        super(referent, queue);
        this.runnable = runnable;
    }

    @Override
    public void run() {
        if (REFS.remove(this)) {
            runnable.run();
        }
    }
}
