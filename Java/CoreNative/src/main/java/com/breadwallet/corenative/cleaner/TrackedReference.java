package com.breadwallet.corenative.cleaner;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;
import java.util.Collections;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/* package */
final class TrackedReference extends PhantomReference<Object> implements Runnable {

    /* package */
    static void create(ReferenceQueue<Object> queue, Object referent, Runnable runnable) {
        REFS.add(new TrackedReference(queue, referent, runnable));
    }

    private static final Set<TrackedReference> REFS = Collections.newSetFromMap(new ConcurrentHashMap<>());

    private final Runnable runnable;

    private TrackedReference(ReferenceQueue<Object> queue, Object referent, Runnable runnable) {
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
