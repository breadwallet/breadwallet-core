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

public enum BRCryptoWalletState {

    CRYPTO_WALLET_STATE_CREATED {
        @Override
        public int toNative() {
            return 0;
        }
    },

    CRYPTO_WALLET_STATE_DELETED {
        @Override
        public int toNative() {
            return 1;
        }
    };

    private static final ImmutableMap<Integer, BRCryptoWalletState> LOOKUP;

    static {
        ImmutableMap.Builder<Integer, BRCryptoWalletState> b = ImmutableMap.builder();

        b.put(CRYPTO_WALLET_STATE_CREATED.toNative(), CRYPTO_WALLET_STATE_CREATED);
        b.put(CRYPTO_WALLET_STATE_DELETED.toNative(), CRYPTO_WALLET_STATE_DELETED);

        LOOKUP = b.build();
    }

    public static BRCryptoWalletState fromNative(int nativeValue) {
        BRCryptoWalletState status = LOOKUP.get(nativeValue);
        checkState(null != status);
        return status;
    }

    public abstract int toNative();
}
