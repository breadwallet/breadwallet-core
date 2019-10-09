/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.google.common.collect.ImmutableMap;

import static com.google.common.base.Preconditions.checkState;

public enum BRCryptoTransferDirection {

    CRYPTO_TRANSFER_SENT {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_TRANSFER_RECEIVED {
        @Override
        public int toNative() {
            return 1;
        }
    },

    CRYPTO_TRANSFER_RECOVERED {
        @Override
        public int toNative() {
            return 2;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoTransferDirection> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoTransferDirection> b = ImmutableMap.builder();

        b.put(CRYPTO_TRANSFER_SENT.toNative(),      CRYPTO_TRANSFER_SENT);
        b.put(CRYPTO_TRANSFER_RECEIVED.toNative(),  CRYPTO_TRANSFER_RECEIVED);
        b.put(CRYPTO_TRANSFER_RECOVERED.toNative(), CRYPTO_TRANSFER_RECOVERED);

        LOOKUP = b.build();
    }

    public static BRCryptoTransferDirection fromNative(int nativeValue) {
        BRCryptoTransferDirection status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
