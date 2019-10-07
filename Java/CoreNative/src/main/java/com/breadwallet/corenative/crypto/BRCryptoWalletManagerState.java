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

public class BRCryptoWalletManagerState extends Structure {

    public int type;
    public u_union u;

    public static class u_union extends Union {

        public disconnected_struct disconnected;

        public static class disconnected_struct extends Structure {

            public BRDisconnectReason reason;

            public disconnected_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("reason");
            }

            public disconnected_struct(BRDisconnectReason reason) {
                super();
                this.reason = reason;
            }

            public disconnected_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends disconnected_struct implements Structure.ByReference {

            }

            public static class ByValue extends disconnected_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(disconnected_struct state) {
            super();
            this.disconnected = state;
            setType(disconnected_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRCryptoWalletManagerState() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("type", "u");
    }

    public BRCryptoWalletManagerState(int type, u_union u) {
        super();
        this.type = type;
        this.u = u;
    }

    public BRCryptoWalletManagerState(Pointer peer) {
        super(peer);
    }

    @Override
    public void read() {
        super.read();
        switch (type){
            case BRCryptoWalletManagerStateType.CRYPTO_WALLET_MANAGER_STATE_DISCONNECTED:
                u.setType(u_union.disconnected_struct.class);
                u.read();
                break;
        }
    }

    public static class ByReference extends BRCryptoWalletManagerState implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoWalletManagerState implements Structure.ByValue {

    }
}
