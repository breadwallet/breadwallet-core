/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/10/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.cleaner;

import java.lang.ref.ReferenceQueue;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class ReferenceCleaner {

    private static final Logger Log = Logger.getLogger(ReferenceCleaner.class.getName());

    /**
     * Register a runnable to be executed once all references to `referent`
     * have been dropped.
     *
     * This method provides an alternative to the `finalize` method, which
     * is deprecated as of JDK9.
     */
    public static void register(Object referent, Runnable runnable) {
        INSTANCE.registerRunnable(referent, runnable);
    }

    private static final ReferenceCleaner INSTANCE = new ReferenceCleaner();

    private final ReferenceQueue<Object> queue;
    private final Thread thread;

    private ReferenceCleaner() {
        this.queue = new ReferenceQueue<>();
        this.thread = new Thread(new ReferenceCleanerRunnable(queue));
        this.thread.setDaemon(true);
        this.thread.setName(getClass().getName());
        this.thread.start();
    }

    private void registerRunnable(Object referent, Runnable runnable) {
        Reference.create(queue, referent, runnable);
    }

    private static class ReferenceCleanerRunnable implements Runnable {

        final ReferenceQueue<Object> queue;

        ReferenceCleanerRunnable(ReferenceQueue<Object> queue) {
            this.queue = queue;
        }

        @Override
        public void run() {
            for (;;) {
                Reference ref;

                try {
                    ref = (Reference) queue.remove();
                } catch (ClassCastException | InterruptedException e) {
                    Log.log(Level.SEVERE, "Error pumping queue", e);
                    continue;
                }

                try {
                    ref.run();
                } catch (Throwable t) {
                    Log.log(Level.SEVERE, "Error cleaning up", t);
                }
            }
        }
    }
}
