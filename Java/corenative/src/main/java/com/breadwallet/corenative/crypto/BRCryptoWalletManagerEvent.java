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

public class BRCryptoWalletManagerEvent extends Structure {

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

            public BRCryptoWalletManagerState oldValue;
            public BRCryptoWalletManagerState newValue;

            public state_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("oldValue", "newValue");
            }

            public state_struct(BRCryptoWalletManagerState oldValue, BRCryptoWalletManagerState newValue) {
                super();
                this.oldValue = oldValue;
                this.newValue = newValue;
            }

            public state_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends state_struct implements Structure.ByReference {

            }

            public static class ByValue extends state_struct implements Structure.ByValue {

            }
        }

        public static class wallet_struct extends Structure {

            public BRCryptoWallet value;

            public wallet_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("value");
            }

            public wallet_struct(BRCryptoWallet value) {
                super();
                this.value = value;
            }

            public wallet_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends wallet_struct implements Structure.ByReference {

            }

            public static class ByValue extends wallet_struct implements Structure.ByValue {

            }
        }

        public static class syncContinues_struct extends Structure {

            public int timestamp;
            public float percentComplete;
            public syncContinues_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("timestamp", "percentComplete");
            }

            public syncContinues_struct(int timestamp, float percentComplete) {
                super();
                this.timestamp = timestamp;
                this.percentComplete = percentComplete;
            }

            public syncContinues_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends syncContinues_struct implements Structure.ByReference {

            }

            public static class ByValue extends syncContinues_struct implements Structure.ByValue {

            }
        }

        public static class syncStopped_struct extends Structure {

            public BRCryptoSyncStoppedReason reason;

            public syncStopped_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("reason");
            }

            public syncStopped_struct(BRCryptoSyncStoppedReason reason) {
                super();
                this.reason = reason;
            }

            public syncStopped_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends syncStopped_struct implements Structure.ByReference {

            }

            public static class ByValue extends syncStopped_struct implements Structure.ByValue {

            }
        }

        public static class syncRecommended_struct extends Structure {

            public int depthEnum;

            public syncRecommended_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("depthEnum");
            }

            public syncRecommended_struct(int depth) {
                super();
                this.depthEnum = depth;
            }

            public syncRecommended_struct(Pointer peer) {
                super(peer);
            }

            public BRCryptoSyncDepth depth() {
                return BRCryptoSyncDepth.fromCore(depthEnum);
            }

            public static class ByReference extends syncRecommended_struct implements Structure.ByReference {

            }

            public static class ByValue extends syncRecommended_struct implements Structure.ByValue {

            }
        }

        public static class blockHeight_struct extends Structure {

            public long value;

            public blockHeight_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("value");
            }

            public blockHeight_struct(long value) {
                super();
                this.value = value;
            }

            public blockHeight_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends blockHeight_struct implements Structure.ByReference {

            }

            public static class ByValue extends blockHeight_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(state_struct state) {
            super();
            this.state = state;
            setType(state_struct.class);
        }

        public u_union(wallet_struct wallet) {
            super();
            this.wallet = wallet;
            setType(wallet_struct.class);
        }

        public u_union(syncContinues_struct syncContinues) {
            super();
            this.syncContinues = syncContinues;
            setType(syncContinues_struct.class);
        }

        public u_union(syncStopped_struct syncStopped) {
            super();
            this.syncStopped = syncStopped;
            setType(syncStopped_struct.class);
        }

        public u_union(syncRecommended_struct syncRecommended) {
            super();
            this.syncRecommended = syncRecommended;
            setType(syncRecommended_struct.class);
        }

        public u_union(blockHeight_struct blockHeight) {
            super();
            this.blockHeight = blockHeight;
            setType(blockHeight_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRCryptoWalletManagerEvent() {
        super();
    }

    public BRCryptoWalletManagerEventType type() {
        return BRCryptoWalletManagerEventType.fromCore(typeEnum);
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("typeEnum", "u");
    }

    public BRCryptoWalletManagerEvent(int type, u_union u) {
        super();
        this.typeEnum = type;
        this.u = u;
    }

    public BRCryptoWalletManagerEvent(Pointer peer) {
        super(peer);
    }

    @Override
    public void read() {
        super.read();
        switch (type()){
            case CRYPTO_WALLET_MANAGER_EVENT_BLOCK_HEIGHT_UPDATED:
                u.setType(u_union.blockHeight_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_CHANGED:
                u.setType(u_union.state_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_CONTINUES:
                u.setType(u_union.syncContinues_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_STOPPED:
                u.setType(u_union.syncStopped_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_SYNC_RECOMMENDED:
                u.setType(u_union.syncRecommended_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_ADDED:
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_CHANGED:
            case CRYPTO_WALLET_MANAGER_EVENT_WALLET_DELETED:
                u.setType(u_union.wallet_struct.class);
                u.read();
                break;
        }
    }

    public static class ByReference extends BRCryptoWalletManagerEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoWalletManagerEvent implements Structure.ByValue {

    }
}
