/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

public enum BRSyncMode {

    SYNC_MODE_BRD_ONLY {
        @Override
        public int toCore() {
            return SYNC_MODE_BRD_ONLY_VALUE;
        }
    },

    SYNC_MODE_BRD_WITH_P2P_SEND {
        @Override
        public int toCore() {
            return SYNC_MODE_BRD_WITH_P2P_SEND_VALUE;
        }
    },

    SYNC_MODE_P2P_WITH_BRD_SYNC {
        @Override
        public int toCore() {
            return SYNC_MODE_P2P_WITH_BRD_SYNC_VALUE;
        }
    },

    SYNC_MODE_P2P_ONLY {
        @Override
        public int toCore() {
            return SYNC_MODE_P2P_ONLY_VALUE;
        }
    };

    private static final int SYNC_MODE_BRD_ONLY_VALUE           = 0;
    private static final int SYNC_MODE_BRD_WITH_P2P_SEND_VALUE  = 1;
    private static final int SYNC_MODE_P2P_WITH_BRD_SYNC_VALUE  = 2;
    private static final int SYNC_MODE_P2P_ONLY_VALUE           = 3;

    public static BRSyncMode fromCore(int nativeValue) {
        switch (nativeValue) {
            case SYNC_MODE_BRD_ONLY_VALUE:          return SYNC_MODE_BRD_ONLY;
            case SYNC_MODE_BRD_WITH_P2P_SEND_VALUE: return SYNC_MODE_BRD_WITH_P2P_SEND;
            case SYNC_MODE_P2P_WITH_BRD_SYNC_VALUE: return SYNC_MODE_P2P_WITH_BRD_SYNC;
            case SYNC_MODE_P2P_ONLY_VALUE:          return SYNC_MODE_P2P_ONLY;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
