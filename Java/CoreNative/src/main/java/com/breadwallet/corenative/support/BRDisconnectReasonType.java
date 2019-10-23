package com.breadwallet.corenative.support;

public enum BRDisconnectReasonType {

    DISCONNECT_REASON_REQUESTED  {
        @Override
        public int toCore() {
            return DISCONNECT_REASON_REQUESTED_VALUE;
        }
    },

    DISCONNECT_REASON_UNKNOWN  {
        @Override
        public int toCore() {
            return DISCONNECT_REASON_UNKNOWN_VALUE;
        }
    },

    DISCONNECT_REASON_POSIX  {
        @Override
        public int toCore() {
            return DISCONNECT_REASON_POSIX_VALUE;
        }
    };

    private static final int DISCONNECT_REASON_REQUESTED_VALUE  = 0;
    private static final int DISCONNECT_REASON_UNKNOWN_VALUE    = 1;
    private static final int DISCONNECT_REASON_POSIX_VALUE      = 2;

    public static BRDisconnectReasonType fromCore(int nativeValue) {
        switch (nativeValue) {
            case DISCONNECT_REASON_REQUESTED_VALUE: return DISCONNECT_REASON_REQUESTED;
            case DISCONNECT_REASON_UNKNOWN_VALUE:   return DISCONNECT_REASON_UNKNOWN;
            case DISCONNECT_REASON_POSIX_VALUE:     return DISCONNECT_REASON_POSIX;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
