/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.support.BRDisconnectReason;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;

import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoWalletManagerState {

    public static BRCryptoWalletManagerState create(Struct struct) {
        BRCryptoWalletManagerState state = null;

        BRCryptoWalletManagerStateType type = struct.type();

        switch (struct.type()) {
            case CRYPTO_WALLET_MANAGER_STATE_CREATED:
            case CRYPTO_WALLET_MANAGER_STATE_DELETED:
            case CRYPTO_WALLET_MANAGER_STATE_SYNCING:
            case CRYPTO_WALLET_MANAGER_STATE_CONNECTED:
                state = new BRCryptoWalletManagerState(type);
                break;
            case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED:
                BRDisconnectReason reason = BRDisconnectReason.create(struct.u.disconnected.reason);

                state = new BRCryptoWalletManagerState(type, reason);
                break;
        }

        checkState(null != state);
        return state;
    }

    public static BRCryptoWalletManagerState create(Pointer ptr) {
        BRCryptoWalletManagerState state = null;

        long offset = STRUCT.offsetOfType();
        BRCryptoWalletManagerStateType type = BRCryptoWalletManagerStateType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case CRYPTO_WALLET_MANAGER_STATE_CREATED:
            case CRYPTO_WALLET_MANAGER_STATE_DELETED:
            case CRYPTO_WALLET_MANAGER_STATE_SYNCING:
            case CRYPTO_WALLET_MANAGER_STATE_CONNECTED:
                state = new BRCryptoWalletManagerState(type);
                break;
            case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED:
                BRDisconnectReason reason = BRDisconnectReason.create(ptr.share(offset + STRUCT.u.disconnected.offsetOfReason()));

                state = new BRCryptoWalletManagerState(type, reason);
                break;
        }

        checkState(null != state);
        return state;
    }

    private static Struct STRUCT = new Struct();

    public final BRCryptoWalletManagerStateType type;
    public final BRCryptoWalletManagerStateDisconnected disconnected;

    private BRCryptoWalletManagerState(BRCryptoWalletManagerStateType type,
                                       BRCryptoWalletManagerStateDisconnected disconnected) {
        this.type = type;
        this.disconnected = disconnected;
    }

    public BRCryptoWalletManagerState(BRCryptoWalletManagerStateType type) {
        this(
                type,
                (BRCryptoWalletManagerStateDisconnected) null
        );
    }

    public BRCryptoWalletManagerState(BRCryptoWalletManagerStateType type,
                                      BRDisconnectReason reason) {
        this(
                type,
                new BRCryptoWalletManagerStateDisconnected(reason)
        );
    }

    public static class BRCryptoWalletManagerStateDisconnected {

        public final BRDisconnectReason reason;

        BRCryptoWalletManagerStateDisconnected(BRDisconnectReason reason) {
            this.reason = reason;
        }
    }

    public static class Struct extends Structure {

        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public disconnected_struct disconnected;

            public static class disconnected_struct extends Structure {

                public BRDisconnectReason.Struct reason;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("reason");
                }

                long offsetOfReason() {
                    return fieldOffset("reason");
                }
            }
        }

        public Struct() {
            super();
        }

        public Struct(Pointer peer) {
            super(peer);
        }

        public BRCryptoWalletManagerStateType type() {
            return BRCryptoWalletManagerStateType.fromCore(typeEnum);
        }

        protected List<String> getFieldOrder() {
            return Arrays.asList("typeEnum", "u");
        }

        @Override
        public void read() {
            super.read();
            switch (type()){
                case CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED:
                    u.setType(u_union.disconnected_struct.class);
                    u.read();
                    break;
            }
        }

        long offsetOfType() {
            return fieldOffset("typeEnum");
        }

        long offsetOfUnion() {
            return fieldOffset("u");
        }

        public static class ByValue extends Struct implements Structure.ByValue {

        }
    }
}
