/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRSyncMode {

    SYNC_MODE_BRD_ONLY {
        @Override
        public int toNative() {
            return 0;
        }
    },

    SYNC_MODE_BRD_WITH_P2P_SEND {
        @Override
        public int toNative() {
            return 1;
        }
    },

    SYNC_MODE_P2P_WITH_BRD_SYNC {
        @Override
        public int toNative() {
            return 2;
        }
    },

    SYNC_MODE_P2P_ONLY {
        @Override
        public int toNative() {
            return 3;
        }
    };

    private static final ImmutableMap<Integer, BRSyncMode> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRSyncMode> b = ImmutableMap.builder();

        b.put(SYNC_MODE_BRD_ONLY.toNative(),          SYNC_MODE_BRD_ONLY);
        b.put(SYNC_MODE_BRD_WITH_P2P_SEND.toNative(), SYNC_MODE_BRD_WITH_P2P_SEND);
        b.put(SYNC_MODE_P2P_WITH_BRD_SYNC.toNative(), SYNC_MODE_P2P_WITH_BRD_SYNC);
        b.put(SYNC_MODE_P2P_ONLY.toNative(),          SYNC_MODE_P2P_ONLY);
        LOOKUP = b.build();
    }

    public static BRSyncMode fromNative(int nativeValue) {
        BRSyncMode status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
