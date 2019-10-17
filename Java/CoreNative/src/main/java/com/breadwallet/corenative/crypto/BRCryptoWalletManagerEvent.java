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
import java.util.Collections;
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

            protected List<String> getFieldOrder() {
                return Arrays.asList("oldValue", "newValue");
            }
        }

        public static class wallet_struct extends Structure {

            public BRCryptoWallet value;

            protected List<String> getFieldOrder() {
                return Arrays.asList("value");
            }
        }

        public static class syncContinues_struct extends Structure {

            public int timestamp;
            public float percentComplete;

            protected List<String> getFieldOrder() {
                return Arrays.asList("timestamp", "percentComplete");
            }
        }

        public static class syncStopped_struct extends Structure {

            public BRSyncStoppedReason reason;

            protected List<String> getFieldOrder() {
                return Collections.singletonList("reason");
            }
        }

        public static class syncRecommended_struct extends Structure {

            public int depthEnum;

            protected List<String> getFieldOrder() {
                return Arrays.asList("depthEnum");
            }

            public BRSyncDepth depth() {
                return BRSyncDepth.fromCore(depthEnum);
            }
        }

        public static class blockHeight_struct extends Structure {

            public long value;

            protected List<String> getFieldOrder() {
                return Collections.singletonList("value");
            }
        }
    }

    public BRCryptoWalletManagerEventType type() {
        return BRCryptoWalletManagerEventType.fromCore(typeEnum);
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
    
    protected List<String> getFieldOrder() {
        return Arrays.asList("typeEnum", "u");
    }

    /* package */
    int offsetOfUnion() {
        return fieldOffset("u");
    }

    public static class ByReference extends BRCryptoWalletManagerEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoWalletManagerEvent implements Structure.ByValue {

    }
}
