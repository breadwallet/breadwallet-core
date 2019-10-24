/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoWalletEvent {

    public static BRCryptoWalletEvent create(Pointer ptr) {
        BRCryptoWalletEvent event = null;

        long offset = STRUCT.offsetOfType();
        BRCryptoWalletEventType type = BRCryptoWalletEventType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case CRYPTO_WALLET_EVENT_CREATED:
            case CRYPTO_WALLET_EVENT_DELETED:
                event = new BRCryptoWalletEvent(type);
                break;
            case CRYPTO_WALLET_EVENT_CHANGED:
                int oldState = ptr.getInt(offset + STRUCT.u.state.offsetOfOldState());
                int newState = ptr.getInt(offset + STRUCT.u.state.offsetOfNewState());

                event = new BRCryptoWalletEvent(type, oldState, newState);
                break;
            case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
            case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
            case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
            case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
                BRCryptoTransfer transfer = new BRCryptoTransfer(ptr.getPointer(offset + STRUCT.u.transfer.offsetOfTransfer()));

                event = new BRCryptoWalletEvent(type, transfer);
                break;
            case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                BRCryptoAmount amount = new BRCryptoAmount(ptr.getPointer(offset + STRUCT.u.balanceUpdated.offsetOfAmount()));

                event = new BRCryptoWalletEvent(type, amount);
                break;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                BRCryptoFeeBasis feeBasisUpdate = new BRCryptoFeeBasis(ptr.getPointer(offset + STRUCT.u.feeBasisUpdated.offsetOfFeeBasis()));

                event = new BRCryptoWalletEvent(type, feeBasisUpdate);
                break;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED:
                int status = ptr.getInt(offset + STRUCT.u.feeBasisEstimated.offsetOfStatus());
                Cookie cookie = new Cookie(ptr.getPointer(offset + STRUCT.u.feeBasisEstimated.offsetOfCookie()));
                BRCryptoFeeBasis feeBasisEstimate = new BRCryptoFeeBasis(ptr.getPointer(offset + STRUCT.u.feeBasisEstimated.offsetOfFeeBasis()));

                event = new BRCryptoWalletEvent(type, status, cookie, feeBasisEstimate);
                break;
        }

        checkState(null != event);
        return event;
    }

    private static Struct STRUCT = new Struct();

    public final BRCryptoWalletEventType type;
    public final BRCryptoWalletEventState state;
    public final BRCryptoWalletEventTransfer transfer;
    public final BRCryptoWalletEventBalanceUpdated balanceUpdated;
    public final BRCryptoWalletEventFeeBasisUpdated feeBasisUpdated;
    public final BRCryptoWalletEventFeeBasisEstimated feeBasisEstimated;

    private BRCryptoWalletEvent(BRCryptoWalletEventType type,
                                BRCryptoWalletEventState state,
                                BRCryptoWalletEventTransfer transfer,
                                BRCryptoWalletEventBalanceUpdated balanceUpdated,
                                BRCryptoWalletEventFeeBasisUpdated feeBasisUpdated,
                                BRCryptoWalletEventFeeBasisEstimated feeBasisEstimated) {
        this.type = type;
        this.transfer = transfer;
        this.state = state;
        this.balanceUpdated = balanceUpdated;
        this.feeBasisUpdated = feeBasisUpdated;
        this.feeBasisEstimated = feeBasisEstimated;
    }

    private BRCryptoWalletEvent(BRCryptoWalletEventType type) {
        this(
                type,
                null,
                null,
                null,
                null,
                null
        );
    }

    private BRCryptoWalletEvent(BRCryptoWalletEventType type,
                                int oldState,
                                int newState) {
        this(
                type,
                new BRCryptoWalletEventState(oldState, newState),
                null,
                null,
                null,
                null
        );
    }

    private BRCryptoWalletEvent(BRCryptoWalletEventType type,
                                BRCryptoTransfer transfer) {
        this(
                type,
                null,
                new BRCryptoWalletEventTransfer(transfer),
                null,
                null,
                null
        );
    }

    private BRCryptoWalletEvent(BRCryptoWalletEventType type,
                                BRCryptoAmount amount) {
        this(
                type,
                null,
                null,
                new BRCryptoWalletEventBalanceUpdated(amount),
                null,
                null
        );
    }

    private BRCryptoWalletEvent(BRCryptoWalletEventType type,
                                BRCryptoFeeBasis feeBasis) {
        this(
                type,
                null,
                null,
                null,
                new BRCryptoWalletEventFeeBasisUpdated(feeBasis),
                null
        );
    }

    public BRCryptoWalletEvent(BRCryptoWalletEventType type, int status, Cookie cookie, BRCryptoFeeBasis feeBasisEstimate) {
        this(
                type,
                null,
                null,
                null,
                null,
                new BRCryptoWalletEventFeeBasisEstimated(status, cookie, feeBasisEstimate)
        );
    }

    public static class BRCryptoWalletEventTransfer {

        public final BRCryptoTransfer value;

        BRCryptoWalletEventTransfer(BRCryptoTransfer value) {
            this.value = value;
        }
    }

    public static class BRCryptoWalletEventState {

        public final BRCryptoWalletState oldState;
        public final BRCryptoWalletState newState;

        BRCryptoWalletEventState(int oldState, int newState) {
            this.oldState = BRCryptoWalletState.fromCore(oldState);
            this.newState = BRCryptoWalletState.fromCore(newState);
        }
    }

    public static class BRCryptoWalletEventBalanceUpdated {

        public final BRCryptoAmount amount;

        BRCryptoWalletEventBalanceUpdated(BRCryptoAmount amount) {
            this.amount = amount;
        }
    }

    public static class BRCryptoWalletEventFeeBasisUpdated {

        public final BRCryptoFeeBasis basis;

        BRCryptoWalletEventFeeBasisUpdated(BRCryptoFeeBasis basis) {
            this.basis = basis;
        }
    }

    public static class BRCryptoWalletEventFeeBasisEstimated {

        public final Cookie cookie;
        public final BRCryptoStatus status;
        public final BRCryptoFeeBasis basis;

        BRCryptoWalletEventFeeBasisEstimated(int status, Cookie cookie, BRCryptoFeeBasis basis) {
            this.status = BRCryptoStatus.fromCore(status);
            this.cookie = cookie;
            this.basis = basis;
        }
    }

    private static class Struct extends Structure {

        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public state_struct state;
            public transfer_struct transfer;
            public balanceUpdated_struct balanceUpdated;
            public feeBasisUpdated_struct feeBasisUpdated;
            public feeBasisEstimated_struct feeBasisEstimated;

            public static class state_struct extends Structure {

                public int oldStateEnum;
                public int newStateEnum;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("oldStateEnum", "newStateEnum");
                }

                long offsetOfOldState() {
                    return fieldOffset("oldStateEnum");
                }

                long offsetOfNewState() {
                    return fieldOffset("newStateEnum");
                }
            }

            public static class transfer_struct extends Structure {

                public BRCryptoTransfer value;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("value");
                }

                long offsetOfTransfer() {
                    return fieldOffset("value");
                }
            }

            public static class balanceUpdated_struct extends Structure {

                public BRCryptoAmount amount;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("amount");
                }

                long offsetOfAmount() {
                    return fieldOffset("amount");
                }
            }

            public static class feeBasisUpdated_struct extends Structure {

                public BRCryptoFeeBasis basis;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("basis");
                }

                long offsetOfFeeBasis() {
                    return fieldOffset("basis");
                }
            }

            public static class feeBasisEstimated_struct extends Structure {

                public int statusEnum;
                public Pointer cookie;
                public BRCryptoFeeBasis basis;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("statusEnum", "cookie", "basis");
                }

                long offsetOfStatus() {
                    return fieldOffset("statusEnum");
                }

                long offsetOfCookie() {
                    return fieldOffset("cookie");
                }

                long offsetOfFeeBasis() {
                    return fieldOffset("basis");
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
