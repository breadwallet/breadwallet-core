package com.breadwallet.corenative.cleaner;

public final class ReferenceCleaner {

    private static final ReferenceDaemon DAEMON = new ReferenceDaemon();

    public static void register(Object referent, Runnable runnable) {
        DAEMON.register(referent, runnable);
    }

    private ReferenceCleaner() {}
}
