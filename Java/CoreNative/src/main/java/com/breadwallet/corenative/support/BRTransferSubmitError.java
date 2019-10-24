/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.support;

import com.breadwallet.corenative.CryptoLibraryDirect;
import com.google.common.base.Optional;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;

import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRTransferSubmitError {

    public static BRTransferSubmitError create(Struct struct) {
        BRTransferSubmitError event = null;

        BRTransferSubmitErrorType type = struct.type();

        switch (type) {
            case TRANSFER_SUBMIT_ERROR_POSIX:
                event = new BRTransferSubmitError(type, struct.u.posix.errnum);
                break;
            case TRANSFER_SUBMIT_ERROR_UNKNOWN:
                event = new BRTransferSubmitError(type);
                break;
        }

        checkState(null != event);
        return event;
    }

    public static BRTransferSubmitError create(Pointer ptr) {
        BRTransferSubmitError event = null;

        long offset = STRUCT.offsetOfType();
        BRTransferSubmitErrorType type = BRTransferSubmitErrorType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case TRANSFER_SUBMIT_ERROR_POSIX:
                int errnum = ptr.getInt(offset + STRUCT.u.posix.offsetOfErrnum());

                event = new BRTransferSubmitError(type, errnum);
                break;
            case TRANSFER_SUBMIT_ERROR_UNKNOWN:
                event = new BRTransferSubmitError(type);
                break;
        }

        checkState(null != event);
        return event;
    }

    private static Struct STRUCT = new Struct();

    public final BRTransferSubmitErrorType type;
    public final BRTransferSubmitErrorPosix posix;

    private BRTransferSubmitError(BRTransferSubmitErrorType type,
                                  BRTransferSubmitErrorPosix posix) {
        this.type = type;
        this.posix = posix;
    }

    public BRTransferSubmitError(BRTransferSubmitErrorType type) {
        this(
                type,
                null
        );
    }

    public BRTransferSubmitError(BRTransferSubmitErrorType type,
                                 int errnum) {
        this(
                type,
                new BRTransferSubmitErrorPosix(errnum)
        );
    }

    public Optional<String> getMessage() {
        // TODO(fix): Cache this?
        Struct core = new Struct(this);
        core.setAutoRead(false);
        core.setAutoWrite(true);
        return core.getMessage();
    }

    public static class BRTransferSubmitErrorPosix {

        public final int errnum;

        BRTransferSubmitErrorPosix(int errnum) {
            this.errnum = errnum;
        }
    }

    public static class Struct extends Structure {
        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public posix_struct posix;

            public static class posix_struct extends Structure {

                public int errnum = 0;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("errnum");
                }

                long offsetOfErrnum() {
                    return fieldOffset("errnum");
                }
            }
        }

        public Struct() {
            super();
        }

        public Struct(Pointer pointer) {
            super(pointer);
        }

        public Struct(BRTransferSubmitError error) {
            this.typeEnum =  error.type.toCore();
            // TODO(fix): Figure out how to get the errnum field populated
        }

        public BRTransferSubmitErrorType type() {
            return BRTransferSubmitErrorType.fromCore(typeEnum);
        }

        protected List<String> getFieldOrder() {
            return Arrays.asList("typeEnum", "u");
        }

        @Override
        public void read() {
            super.read();
            if (type() == BRTransferSubmitErrorType.TRANSFER_SUBMIT_ERROR_POSIX) {
                u.setType(u_union.posix_struct.class);
                u.read();
            }
        }

        @Override
        public void write() {
            if (type() == BRTransferSubmitErrorType.TRANSFER_SUBMIT_ERROR_POSIX) {
                u.setType(u_union.posix_struct.class);
                u.write();
            }
            super.write();
        }

        long offsetOfType() {
            return fieldOffset("typeEnum");
        }

        long offsetOfUnion() {
            return fieldOffset("u");
        }

        public Optional<String> getMessage() {
            Pointer ptr = CryptoLibraryDirect.BRTransferSubmitErrorGetMessage(this);
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
}
