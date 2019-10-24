/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.support.BRSyncDepth;
import com.breadwallet.corenative.support.BRSyncStoppedReason;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

import static com.google.common.base.Preconditions.checkState;

public class BRCryptoWalletManagerEvent {

    public static BRCryptoWalletManagerEvent create(Pointer ptr) {
        BRCryptoWalletManagerEvent event = null;

        long offset = STRUCT.offsetOfType();
        BRCryptoWalletManagerEventType type = BRCryptoWalletManagerEventType.fromCore(ptr.getInt(offset));

        offset = STRUCT.offsetOfUnion();
        switch (type) {
            case CRYPTO_WALLET_MANAGER_EVENT_CREATED:
            case CRYPTO_WALLET_MANAGER_EVENT_DELETED:
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STARTED:
                event = new BRCryptoWalletManagerEvent(type);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                long height = ptr.getLong(offset + STRUCT.u.blockHeight.offsetOfHeight());

                event = new BRCryptoWalletManagerEvent(type, height);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                BRCryptoWalletManagerState oldState = BRCryptoWalletManagerState.create(ptr.share(offset + STRUCT.u.state.offsetOfOldState()));
                BRCryptoWalletManagerState newState = BRCryptoWalletManagerState.create(ptr.share(offset + STRUCT.u.state.offsetOfNewState()));

                event = new BRCryptoWalletManagerEvent(type, oldState, newState);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                int timestamp = ptr.getInt(offset + STRUCT.u.syncContinues.offsetOfTimestamp());
                float percentComplete = ptr.getFloat(offset + STRUCT.u.syncContinues.offsetOfPercentComplete());

                event = new BRCryptoWalletManagerEvent(type, timestamp, percentComplete);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                BRSyncStoppedReason reason = BRSyncStoppedReason.create(ptr.share(offset + STRUCT.u.syncStopped.offsetOfReason()));

                event = new BRCryptoWalletManagerEvent(type, reason);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED:
                BRSyncDepth depth = BRSyncDepth.fromCore(ptr.getInt(offset + STRUCT.u.syncRecommended.offsetOfDepth()));

                event = new BRCryptoWalletManagerEvent(type, depth);
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                BRCryptoWallet wallet = new BRCryptoWallet(ptr.getPointer(offset + STRUCT.u.wallet.offsetOfWallet()));

                event = new BRCryptoWalletManagerEvent(type, wallet);
                break;
        }

        checkState(null != event);
        return event;
    }

    private static Struct STRUCT = new Struct();

    public final BRCryptoWalletManagerEventType type;
    public final BRCryptoWalletManagerEventState state;
    public final BRCryptoWalletManagerEventSyncContinues syncContinues;
    public final BRCryptoWalletManagerEventSyncStopped syncStopped;
    public final BRCryptoWalletManagerEventSyncRecommended syncRecommended;
    public final BRCryptoWalletManagerEventBlockHeight blockHeight;
    public final BRCryptoWalletManagerEventWallet wallet;

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       BRCryptoWalletManagerEventState state,
                                       BRCryptoWalletManagerEventSyncContinues syncContinues,
                                       BRCryptoWalletManagerEventSyncStopped syncStopped,
                                       BRCryptoWalletManagerEventSyncRecommended syncRecommended,
                                       BRCryptoWalletManagerEventBlockHeight blockHeight,
                                       BRCryptoWalletManagerEventWallet wallet) {
        this.type = type;
        this.state = state;
        this.syncContinues = syncContinues;
        this.syncStopped = syncStopped;
        this.syncRecommended = syncRecommended;
        this.blockHeight = blockHeight;
        this.wallet = wallet;
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type) {
        this(
                type,
                null,
                null,
                null,
                null,
                null,
                null
        );

    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       BRCryptoWalletManagerState oldState,
                                       BRCryptoWalletManagerState newState) {
        this(
                type,
                new BRCryptoWalletManagerEventState(oldState, newState),
                null,
                null,
                null,
                null,
                null
        );
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       int timestamp,
                                       float percentComplete) {
        this(
                type,
                null,
                new BRCryptoWalletManagerEventSyncContinues(timestamp, percentComplete),
                null,
                null,
                null,
                null
        );
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       BRSyncStoppedReason reason) {
        this(
                type,
                null,
                null,
                new BRCryptoWalletManagerEventSyncStopped(reason),
                null,
                null,
                null
        );
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       BRSyncDepth depth) {
        this(
                type,
                null,
                null,
                null,
                new BRCryptoWalletManagerEventSyncRecommended(depth),
                null,
                null
        );
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       long height) {
        this(
                type,
                null,
                null,
                null,
                null,
                new BRCryptoWalletManagerEventBlockHeight(height),
                null
        );
    }

    private BRCryptoWalletManagerEvent(BRCryptoWalletManagerEventType type,
                                       BRCryptoWallet wallet) {
        this(
                type,
                null,
                null,
                null,
                null,
                null,
                new BRCryptoWalletManagerEventWallet(wallet)
        );
    }

    public static class BRCryptoWalletManagerEventState {

        public final BRCryptoWalletManagerState oldState;
        public final BRCryptoWalletManagerState newState;

        BRCryptoWalletManagerEventState(BRCryptoWalletManagerState oldState, BRCryptoWalletManagerState newState) {
            this.oldState = oldState;
            this.newState = newState;
        }
    }

    public static class BRCryptoWalletManagerEventSyncContinues {

        public final int timestamp;
        public final float percentComplete;

        BRCryptoWalletManagerEventSyncContinues(int timestamp, float percentComplete) {
            this.timestamp = timestamp;
            this.percentComplete = percentComplete;
        }
    }

    public static class BRCryptoWalletManagerEventSyncStopped {

        public final BRSyncStoppedReason reason;

        BRCryptoWalletManagerEventSyncStopped(BRSyncStoppedReason reason) {
            this.reason = reason;
        }
    }

    public static class BRCryptoWalletManagerEventSyncRecommended {

        public final BRSyncDepth depth;

        BRCryptoWalletManagerEventSyncRecommended(BRSyncDepth depth) {
            this.depth = depth;
        }
    }

    public static class BRCryptoWalletManagerEventBlockHeight {

        public final long value;

        BRCryptoWalletManagerEventBlockHeight(long value) {
            this.value = value;
        }
    }

    public static class BRCryptoWalletManagerEventWallet {

        public final BRCryptoWallet value;

        BRCryptoWalletManagerEventWallet(BRCryptoWallet value) {
            this.value = value;
        }
    }

    public static class Struct extends Structure {

        public int typeEnum;
        public u_union u;

        public static class u_union extends Union {

            public state_struct state;
            public wallet_struct wallet;
            public syncContinues_struct syncContinues;
            public syncStopped_struct syncStopped;
            public syncRecommended_struct syncRecommended;
            public blockHeight_struct blockHeight;

            public static class state_struct extends Structure {

                public BRCryptoWalletManagerState.Struct oldValue;
                public BRCryptoWalletManagerState.Struct newValue;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("oldValue", "newValue");
                }

                long offsetOfOldState() {
                    return fieldOffset("oldValue");
                }

                long offsetOfNewState() {
                    return fieldOffset("newValue");
                }
            }

            public static class wallet_struct extends Structure {

                public BRCryptoWallet value;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("value");
                }

                long offsetOfWallet() {
                    return fieldOffset("value");
                }
            }

            public static class syncContinues_struct extends Structure {

                public int timestamp;
                public float percentComplete;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("timestamp", "percentComplete");
                }

                long offsetOfTimestamp() {
                    return fieldOffset("timestamp");
                }

                long offsetOfPercentComplete() {
                    return fieldOffset("percentComplete");
                }
            }

            public static class syncStopped_struct extends Structure {

                public BRSyncStoppedReason.Struct reason;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("reason");
                }

                long offsetOfReason() {
                    return fieldOffset("reason");
                }
            }

            public static class syncRecommended_struct extends Structure {

                public int depthEnum;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("depthEnum");
                }

                long offsetOfDepth() {
                    return fieldOffset("depthEnum");
                }
            }

            public static class blockHeight_struct extends Structure {

                public long value;

                protected List<String> getFieldOrder() {
                    return Arrays.asList("value");
                }

                long offsetOfHeight() {
                    return fieldOffset("value");
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
