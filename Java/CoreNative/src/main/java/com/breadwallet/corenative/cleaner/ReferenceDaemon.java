package com.breadwallet.corenative.cleaner;

import android.util.Log;

import java.lang.ref.ReferenceQueue;

/* package */
final class ReferenceDaemon {

    private static final String TAG = ReferenceDaemon.class.getName();

    private final ReferenceQueue<Object> queue;
    private final ReferenceCleanerRunnable runnable;
    private final Thread thread;

    /* package */
    ReferenceDaemon() {
        this.queue = new ReferenceQueue<>();
        this.runnable = new ReferenceCleanerRunnable(queue);
        this.thread = new Thread(runnable);
        this.thread.setDaemon(true);
        this.thread.start();
    }

    /* package */
    void register(Object referent, Runnable runnable) {
        TrackedReference.create(queue, referent, runnable);
    }

    /* package */
    void kill() {
        runnable.kill();
        thread.interrupt();
    }

    private static class ReferenceCleanerRunnable implements Runnable {

        final ReferenceQueue<Object> queue;
        boolean killed;

        ReferenceCleanerRunnable(ReferenceQueue<Object> queue) {
            this.queue = queue;
            this.killed = false;
        }

        void kill() {
            killed = true;
        }

        @Override
        public void run() {
            while(!killed) {
                TrackedReference ref;

                try {
                    ref = (TrackedReference) queue.remove();
                } catch (ClassCastException | InterruptedException e) {
                    Log.e(TAG, "Error pumping queue", e);
                    continue;
                }

                try {
                    ref.run();
                } catch (Throwable t) {
                    Log.e(TAG, "Error cleaning up", t);
                }
            }
        }
    }
}
