/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

public enum BRSyncDepth {

    SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND {
        @Override
        public int toCore() {
            return SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE;
        }
    },

    SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK {
        @Override
        public int toCore() {
            return SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE;
        }
    },

    SYNC_DEPTH_FROM_CREATION {
        @Override
        public int toCore() {
            return SYNC_DEPTH_FROM_CREATION_VALUE;
        }
    };

    private static final int SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE  = 0;
    private static final int SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE   = 1;
    private static final int SYNC_DEPTH_FROM_CREATION_VALUE             = 2;

    public static BRSyncDepth fromCore(int nativeValue) {
        switch (nativeValue) {
            case SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND_VALUE: return SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND;
            case SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK_VALUE:  return SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK;
            case SYNC_DEPTH_FROM_CREATION_VALUE:            return SYNC_DEPTH_FROM_CREATION;
            default: throw new IllegalArgumentException("Invalid core value");
        }
    }

    public abstract int toCore();
}
