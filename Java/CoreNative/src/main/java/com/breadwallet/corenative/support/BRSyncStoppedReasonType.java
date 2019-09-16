package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRSyncStoppedReasonType {

    SYNC_STOPPED_REASON_COMPLETE  {
        @Override
        public int toNative() {
            return 0;
        }
    },

    SYNC_STOPPED_REASON_REQUESTED  {
        @Override
        public int toNative() {
            return 1;
        }
    },

    SYNC_STOPPED_REASON_UNKNOWN  {
        @Override
        public int toNative() {
            return 2;
        }
    },

    SYNC_STOPPED_REASON_POSIX  {
        @Override
        public int toNative() {
            return 3;
        }
    };

    private static final ImmutableMap<Integer, BRSyncStoppedReasonType> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRSyncStoppedReasonType> b = ImmutableMap.builder();

        b.put(SYNC_STOPPED_REASON_COMPLETE.toNative(),          SYNC_STOPPED_REASON_COMPLETE);
        b.put(SYNC_STOPPED_REASON_REQUESTED.toNative(),         SYNC_STOPPED_REASON_REQUESTED);
        b.put(SYNC_STOPPED_REASON_UNKNOWN.toNative(),           SYNC_STOPPED_REASON_UNKNOWN);
        b.put(SYNC_STOPPED_REASON_POSIX.toNative(),             SYNC_STOPPED_REASON_POSIX);
        LOOKUP = b.build();
    }

    public static BRSyncStoppedReasonType fromNative(int nativeValue) {
        BRSyncStoppedReasonType status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
