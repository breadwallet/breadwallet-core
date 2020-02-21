/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;

import java.util.Arrays;
import java.util.List;

import javax.annotation.Nullable;

public class BRCryptoTransferState extends Structure {

    public int typeEnum;
    public u_union u;

    public static class u_union extends Union {

        public included_struct included;
        public errored_struct errored;

        public static class included_struct extends Structure {
            private static int CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE = 16;

            public long blockNumber;
            public long transactionIndex;
            public long timestamp;
            public BRCryptoFeeBasis feeBasis;
            public int success = BRCryptoBoolean.CRYPTO_TRUE;
            public byte[] error = new byte[CRYPTO_TRANSFER_INCLUDED_ERROR_SIZE + 1];

            public included_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("blockNumber", "transactionIndex", "timestamp", "feeBasis", "success", "error");
            }

            public included_struct(long blockNumber, long transactionIndex, long timestamp, BRCryptoFeeBasis feeBasis, int success, byte[] error) {
                super();
                this.blockNumber = blockNumber;
                this.transactionIndex = transactionIndex;
                this.timestamp = timestamp;
                this.feeBasis = feeBasis;
                this.success = success;
                this.error = error;
            }

            public included_struct(Pointer peer) {
                super(peer);
            }

            public @Nullable String getError () {
                return getSuccess() ? null : Native.toString(this.error);
            }

            public boolean getSuccess () {
                return BRCryptoBoolean.CRYPTO_TRUE == this.success;
            }

            public static class ByReference extends included_struct implements Structure.ByReference {

            }
            public static class ByValue extends included_struct implements Structure.ByValue {

            }
        }

        public static class errored_struct extends Structure {

            public BRCryptoTransferSubmitError error;

            public errored_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("error");
            }

            public errored_struct(BRCryptoTransferSubmitError error) {
                super();
                this.error = error;
            }

            public errored_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends errored_struct implements Structure.ByReference {

            }

            public static class ByValue extends errored_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(included_struct state) {
            super();
            this.included = state;
            setType(included_struct.class);
        }

        public u_union(errored_struct confirmation) {
            super();
            this.errored = confirmation;
            setType(errored_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRCryptoTransferState() {
        super();
    }

    public BRCryptoTransferStateType type() {
        return BRCryptoTransferStateType.fromCore(typeEnum);
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("typeEnum", "u");
    }

    public BRCryptoTransferState(int type, u_union u) {
        super();
        this.typeEnum = type;
        this.u = u;
    }

    public BRCryptoTransferState(Pointer peer) {
        super(peer);
    }

    @Override
    public void read() {
        super.read();
        switch (type()){
            case CRYPTO_TRANSFER_STATE_INCLUDED:
                u.setType(u_union.included_struct.class);
                u.read();
                break;
            case CRYPTO_TRANSFER_STATE_ERRORED:
                u.setType(u_union.errored_struct.class);
                u.read();
                break;
        }
    }

    public static class ByReference extends BRCryptoTransferState implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoTransferState implements Structure.ByValue {

    }
}
