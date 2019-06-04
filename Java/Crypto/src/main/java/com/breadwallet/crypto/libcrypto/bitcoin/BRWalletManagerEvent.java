package com.breadwallet.crypto.libcrypto.bitcoin;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

public class BRWalletManagerEvent extends Structure {

    public int type;
    public u_union u;

    public static class u_union extends Union {

        public syncStopped_struct syncStopped;
        public blockHeightUpdated_struct blockHeightUpdated;

        public static class syncStopped_struct extends Structure {

            public int error;

            public syncStopped_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("error");
            }

            public syncStopped_struct(int error) {
                super();
                this.error = error;
            }

            public syncStopped_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends syncStopped_struct implements Structure.ByReference {

            }

            public static class ByValue extends syncStopped_struct implements Structure.ByValue {

            }
        }

        public static class blockHeightUpdated_struct extends Structure {

            public int value;

            public blockHeightUpdated_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("value");
            }

            public blockHeightUpdated_struct(int value) {
                super();
                this.value = value;
            }

            public blockHeightUpdated_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends blockHeightUpdated_struct implements Structure.ByReference {

            }

            public static class ByValue extends blockHeightUpdated_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(syncStopped_struct syncStopped) {
            super();
            this.syncStopped = syncStopped;
            setType(syncStopped_struct.class);
        }

        public u_union(blockHeightUpdated_struct blockHeightUpdated) {
            super();
            this.blockHeightUpdated = blockHeightUpdated;
            setType(blockHeightUpdated_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRWalletManagerEvent() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("type", "u");
    }

    public BRWalletManagerEvent(int type, u_union u) {
        super();
        this.type = type;
        this.u = u;
    }

    public BRWalletManagerEvent(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRWalletManagerEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRWalletManagerEvent implements Structure.ByValue {

    }

    @Override
    public void read() {
        super.read();
        switch (type){
            case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_BLOCK_HEIGHT_UPDATED:
                u.setType(u_union.blockHeightUpdated_struct.class);
                u.read();
                break;
            case BRWalletManagerEventType.BITCOIN_WALLET_MANAGER_SYNC_STOPPED:
                u.setType(u_union.syncStopped_struct.class);
                u.read();
                break;
        }
    }
}
