/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoTransferEvent {

    public static BRCryptoTransferEvent create(Pointer ptr) {
        BRCryptoTransferEvent event = null;

        long offset = STRUCT.offsetOfType();
        BRCryptoTransferEventType type = BRCryptoTransferEventType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case CRYPTO_TRANSFER_EVENT_CREATED:
            case CRYPTO_TRANSFER_EVENT_DELETED:
                event = new BRCryptoTransferEvent(type);
                break;
            case CRYPTO_TRANSFER_EVENT_CHANGED:
                BRCryptoTransferState oldState = BRCryptoTransferState.create(ptr.share(offset + STRUCT.u.state.offsetOfOldState()));
                BRCryptoTransferState newState = BRCryptoTransferState.create(ptr.share(offset + STRUCT.u.state.offsetOfNewState()));

                event = new BRCryptoTransferEvent(type, oldState, newState);
                break;
        }

        checkState(null != event);
        return event;
    }

    private static Struct STRUCT = new Struct();

    public final BRCryptoTransferEventType type;
    public final BRCryptoTransferEventState state;

    private BRCryptoTransferEvent(BRCryptoTransferEventType type,
                                  BRCryptoTransferEventState state) {
        this.type = type;
        this.state = state;
    }

    public BRCryptoTransferEvent(BRCryptoTransferEventType type) {
        this(
                type,
                null
        );
    }

    public BRCryptoTransferEvent(BRCryptoTransferEventType type,
                                 BRCryptoTransferState oldState,
                                 BRCryptoTransferState newState) {
        this(
                type,
                new BRCryptoTransferEventState(oldState, newState)
        );
    }

    public static class BRCryptoTransferEventState {

        public final BRCryptoTransferState oldState;
        public final BRCryptoTransferState newState;

        BRCryptoTransferEventState(BRCryptoTransferState oldState, BRCryptoTransferState newState) {
            this.oldState = oldState;
            this.newState = newState;
        }
    }

    private static class Struct extends Structure {

        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public u_union.state_struct state;

            public static class state_struct extends Structure {

                public BRCryptoTransferState.Struct oldState;
                public BRCryptoTransferState.Struct newState;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("oldState", "newState");
                }

                long offsetOfOldState() {
                    return fieldOffset("oldState");
                }

                long offsetOfNewState() {
                    return fieldOffset("newState");
                }
            }
        }

        protected List<String> getFieldOrder() {
            return Arrays.asList("typeEnum", "u");
        }

        long offsetOfType() {
            return fieldOffset("typeEnum");
        }

        long offsetOfUnion() {
            return fieldOffset("u");
        }
    }
}
