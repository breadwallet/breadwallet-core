/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRTransferSubmitErrorType {

    TRANSFER_SUBMIT_ERROR_UNKNOWN  {
        @Override
        public int toNative() {
            return 0;
        }
    },

    TRANSFER_SUBMIT_ERROR_POSIX  {
        @Override
        public int toNative() {
            return 1;
        }
    };

    private static final ImmutableMap<Integer, BRTransferSubmitErrorType> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRTransferSubmitErrorType> b = ImmutableMap.builder();

        b.put(TRANSFER_SUBMIT_ERROR_UNKNOWN.toNative(),       TRANSFER_SUBMIT_ERROR_UNKNOWN);
        b.put(TRANSFER_SUBMIT_ERROR_POSIX.toNative(),         TRANSFER_SUBMIT_ERROR_POSIX);
        LOOKUP = b.build();
    }

    public static BRTransferSubmitErrorType fromNative(int nativeValue) {
        BRTransferSubmitErrorType status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
