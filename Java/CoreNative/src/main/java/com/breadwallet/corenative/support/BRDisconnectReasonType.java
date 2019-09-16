package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRDisconnectReasonType {

    DISCONNECT_REASON_TEARDOWN  {
        @Override
        public int toNative() {
            return 0;
        }
    },

    DISCONNECT_REASON_UNKNOWN  {
        @Override
        public int toNative() {
            return 1;
        }
    },

    DISCONNECT_REASON_POSIX  {
        @Override
        public int toNative() {
            return 2;
        }
    };

    private static final ImmutableMap<Integer, BRDisconnectReasonType> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRDisconnectReasonType> b = ImmutableMap.builder();

        b.put(DISCONNECT_REASON_TEARDOWN.toNative(),        DISCONNECT_REASON_TEARDOWN);
        b.put(DISCONNECT_REASON_UNKNOWN.toNative(),         DISCONNECT_REASON_UNKNOWN);
        b.put(DISCONNECT_REASON_POSIX.toNative(),           DISCONNECT_REASON_POSIX);
        LOOKUP = b.build();
    }

    public static BRDisconnectReasonType fromNative(int nativeValue) {
        BRDisconnectReasonType status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
