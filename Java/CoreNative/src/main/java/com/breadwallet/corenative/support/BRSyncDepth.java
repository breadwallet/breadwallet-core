package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRSyncDepth {

    SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND {
        @Override
        public int toNative() {
            return 0;
        }
    },

    SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK {
        @Override
        public int toNative() {
            return 1;
        }
    },

    SYNC_DEPTH_FROM_CREATION {
        @Override
        public int toNative() {
            return 2;
        }
    };

    private static final ImmutableMap<Integer, BRSyncDepth> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRSyncDepth> b = ImmutableMap.builder();

        b.put(SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND.toNative(), SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND);
        b.put(SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK.toNative(),  SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK);
        b.put(SYNC_DEPTH_FROM_CREATION.toNative(),            SYNC_DEPTH_FROM_CREATION);

        LOOKUP = b.build();
    }

    public static BRSyncDepth fromNative(int nativeValue) {
        BRSyncDepth status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
