package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRSyncDepth {

    SYNC_DEPTH_LOW {
        @Override
        public int toNative() {
            return 0;
        }
    },

    SYNC_DEPTH_MEDIUM {
        @Override
        public int toNative() {
            return 1;
        }
    },

    SYNC_DEPTH_HIGH {
        @Override
        public int toNative() {
            return 2;
        }
    };

    private static final ImmutableMap<Integer, BRSyncDepth> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRSyncDepth> b = ImmutableMap.builder();

        b.put(SYNC_DEPTH_LOW.toNative(),    SYNC_DEPTH_LOW);
        b.put(SYNC_DEPTH_MEDIUM.toNative(), SYNC_DEPTH_MEDIUM);
        b.put(SYNC_DEPTH_HIGH.toNative(),   SYNC_DEPTH_HIGH);

        LOOKUP = b.build();
    }

    public static BRSyncDepth fromNative(int nativeValue) {
        BRSyncDepth status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
