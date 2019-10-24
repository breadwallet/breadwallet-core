/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.support.BRTransferSubmitError;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;

import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoTransferState {

    public static BRCryptoTransferState create(Struct struct) {
        BRCryptoTransferState state = null;

        BRCryptoTransferStateType type = struct.type();

        switch (struct.type()) {
            case CRYPTO_TRANSFER_STATE_SIGNED:
            case CRYPTO_TRANSFER_STATE_CREATED:
            case CRYPTO_TRANSFER_STATE_DELETED:
            case CRYPTO_TRANSFER_STATE_SUBMITTED:
                state = new BRCryptoTransferState(type);
                break;
            case CRYPTO_TRANSFER_STATE_ERRORED:
                BRTransferSubmitError error = BRTransferSubmitError.create(struct.u.errored.error);

                state = new BRCryptoTransferState(type, error);
                break;
            case CRYPTO_TRANSFER_STATE_INCLUDED:
                long blockNumber = struct.u.included.blockNumber;
                long transactionIndex = struct.u.included.transactionIndex;
                long timestamp = struct.u.included.timestamp;
                BRCryptoAmount fee = struct.u.included.fee;

                state = new BRCryptoTransferState(type, blockNumber, transactionIndex, timestamp, fee);
                break;
        }

        checkState(null != state);
        return state;
    }

    public static BRCryptoTransferState create(Pointer ptr) {
        BRCryptoTransferState state = null;

        long offset = STRUCT.offsetOfType();
        BRCryptoTransferStateType type = BRCryptoTransferStateType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case CRYPTO_TRANSFER_STATE_SIGNED:
            case CRYPTO_TRANSFER_STATE_CREATED:
            case CRYPTO_TRANSFER_STATE_DELETED:
            case CRYPTO_TRANSFER_STATE_SUBMITTED:
                state = new BRCryptoTransferState(type);
                break;
            case CRYPTO_TRANSFER_STATE_ERRORED:
                BRTransferSubmitError error = BRTransferSubmitError.create(ptr.share(offset + STRUCT.u.errored.offsetOfError()));

                state = new BRCryptoTransferState(type, error);
                break;
            case CRYPTO_TRANSFER_STATE_INCLUDED:
                long blockNumber = ptr.getLong(offset + STRUCT.u.included.offsetOfBlockNumber());
                long transactionIndex = ptr.getLong(offset + STRUCT.u.included.offsetOfTransactionIndex());
                long timestamp = ptr.getLong(offset + STRUCT.u.included.offsetOfTimestamp());
                BRCryptoAmount fee = new BRCryptoAmount(ptr.getPointer(offset + STRUCT.u.included.offsetOfFee()));

                state = new BRCryptoTransferState(type, blockNumber, transactionIndex, timestamp, fee);
                break;
        }

        checkState(null != state);
        return state;
    }

    private static Struct STRUCT = new Struct();

    public final BRCryptoTransferStateType type;
    public final BRCryptoTransferStateIncluded included;
    public final BRCryptoTransferStateErrored errored;

    private BRCryptoTransferState(BRCryptoTransferStateType type,
                                  BRCryptoTransferStateIncluded included,
                                  BRCryptoTransferStateErrored errored) {
        this.type = type;
        this.included = included;
        this.errored = errored;
    }

    public BRCryptoTransferState(BRCryptoTransferStateType type) {
        this(
                type,
                null,
                null
        );
    }

    public BRCryptoTransferState(BRCryptoTransferStateType type,
                                 long blockNumber,
                                 long transactionIndex,
                                 long timestamp,
                                 BRCryptoAmount fee) {
        this(
                type,
                new BRCryptoTransferStateIncluded(blockNumber, transactionIndex, timestamp, fee),
                null
        );
    }

    public BRCryptoTransferState(BRCryptoTransferStateType type,
                                 BRTransferSubmitError error) {
        this(
                type,
                null,
                new BRCryptoTransferStateErrored(error)
        );
    }

    public static class BRCryptoTransferStateIncluded {

        public final long blockNumber;
        public final long transactionIndex;
        public final long timestamp;
        public final BRCryptoAmount fee;

        BRCryptoTransferStateIncluded(long blockNumber,
                                      long transactionIndex,
                                      long timestamp,
                                      BRCryptoAmount fee) {
            this.blockNumber = blockNumber;
            this.transactionIndex = transactionIndex;
            this.timestamp = timestamp;
            this.fee = fee;
        }
    }

    public static class BRCryptoTransferStateErrored {

        public final BRTransferSubmitError error;

        BRCryptoTransferStateErrored(BRTransferSubmitError error) {
            this.error = error;
        }
    }

    public static class Struct extends Structure {
        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public included_struct included;
            public errored_struct errored;

            public static class included_struct extends Structure {

                public long blockNumber;
                public long transactionIndex;
                public long timestamp;
                public BRCryptoAmount fee;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("blockNumber", "transactionIndex", "timestamp", "fee");
                }

                long offsetOfBlockNumber() {
                    return fieldOffset("blockNumber");
                }

                long offsetOfTransactionIndex() {
                    return fieldOffset("transactionIndex");
                }

                long offsetOfTimestamp() {
                    return fieldOffset("timestamp");
                }

                long offsetOfFee() {
                    return fieldOffset("fee");
                }
            }

            public static class errored_struct extends Structure {

                public BRTransferSubmitError.Struct error;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("error");
                }

                long offsetOfError() {
                    return fieldOffset("error");
                }
            }
        }

        public Struct() {
            super();
        }

        public Struct(Pointer peer) {
            super(peer);
        }

        public BRCryptoTransferStateType type() {
            return BRCryptoTransferStateType.fromCore(typeEnum);
        }

        protected List<String> getFieldOrder() {
            return Arrays.asList("typeEnum", "u");
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
