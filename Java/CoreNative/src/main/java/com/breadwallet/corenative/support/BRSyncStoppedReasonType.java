package com.breadwallet.corenative.support;

public enum BRSyncStoppedReasonType {

    SYNC_STOPPED_REASON_COMPLETE  {
        @Override
        public int toNative() {
            return SYNC_STOPPED_REASON_COMPLETE_VALUE;
        }
    },

    SYNC_STOPPED_REASON_REQUESTED  {
        @Override
        public int toNative() {
            return SYNC_STOPPED_REASON_REQUESTED_VALUE;
        }
    },

    SYNC_STOPPED_REASON_UNKNOWN  {
        @Override
        public int toNative() {
            return SYNC_STOPPED_REASON_UNKNOWN_VALUE;
        }
    },

    SYNC_STOPPED_REASON_POSIX  {
        @Override
        public int toNative() {
            return SYNC_STOPPED_REASON_POSIX_VALUE;
        }
    };

    private static final int SYNC_STOPPED_REASON_COMPLETE_VALUE  = 0;
    private static final int SYNC_STOPPED_REASON_REQUESTED_VALUE = 1;
    private static final int SYNC_STOPPED_REASON_UNKNOWN_VALUE   = 2;
    private static final int SYNC_STOPPED_REASON_POSIX_VALUE     = 3;

    public static BRSyncStoppedReasonType fromNative(int nativeValue) {
        switch (nativeValue) {
            case SYNC_STOPPED_REASON_COMPLETE_VALUE:  return SYNC_STOPPED_REASON_COMPLETE;
            case SYNC_STOPPED_REASON_REQUESTED_VALUE: return SYNC_STOPPED_REASON_REQUESTED;
            case SYNC_STOPPED_REASON_UNKNOWN_VALUE:   return SYNC_STOPPED_REASON_UNKNOWN;
            case SYNC_STOPPED_REASON_POSIX_VALUE:     return SYNC_STOPPED_REASON_POSIX;
            default: throw new IllegalArgumentException("Invalid native value");
        }
    }

    public abstract int toNative();
}
