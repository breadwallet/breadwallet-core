/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;

import java.util.Arrays;
import java.util.List;

public class BRCryptoSyncStoppedReason extends Structure {

    public int typeEnum;
    public u_union u;

    public static class u_union extends Union {

        public posix_struct posix;

        public static class posix_struct extends Structure {

            public int errnum = 0;

            public posix_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("errnum");
            }

            public posix_struct(int errnum) {
                super();
                this.errnum = errnum;
            }

            public posix_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends posix_struct implements Structure.ByReference {

            }

            public static class ByValue extends posix_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(posix_struct state) {
            super();
            this.posix = state;
            setType(posix_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRCryptoSyncStoppedReason() {
        super();
    }

    public BRCryptoSyncStoppedReasonType type() {
        return BRCryptoSyncStoppedReasonType.fromCore(typeEnum);
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("typeEnum", "u");
    }

    public BRCryptoSyncStoppedReason(int type, u_union u) {
        super();
        this.typeEnum = type;
        this.u = u;
    }

    public BRCryptoSyncStoppedReason(Pointer peer) {
        super(peer);
    }

    @Override
    public void read() {
        super.read();
        if (type() == BRCryptoSyncStoppedReasonType.CRYPTO_SYNC_STOPPED_REASON_POSIX)
            u.setType(u_union.posix_struct.class);
        u.read();
    }

    public Optional<String> getMessage() {
        Pointer ptr = CryptoLibraryDirect.cryptoSyncStoppedReasonGetMessage(this);
        try {
            return Optional.fromNullable(
                    ptr
            ).transform(
                    a -> a.getString(0, "UTF-8")
            );
        } finally {
            if (ptr != null) Native.free(Pointer.nativeValue(ptr));
        }
    }
}
