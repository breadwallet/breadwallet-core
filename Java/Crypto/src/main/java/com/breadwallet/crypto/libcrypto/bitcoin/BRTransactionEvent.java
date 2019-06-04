package com.breadwallet.crypto.libcrypto.bitcoin;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

public class BRTransactionEvent extends Structure {

    public int type;
    public u_union u;

    public static class u_union extends Union {

        public updated_struct updated;

        public static class updated_struct extends Structure {

            public int blockHeight;
            public int timestamp;

            public updated_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("blockHeight", "timestamp");
            }

            public updated_struct(int blockHeight, int timestamp) {
                super();
                this.blockHeight = blockHeight;
                this.timestamp = timestamp;
            }

            public updated_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends updated_struct implements Structure.ByReference {

            }

            public static class ByValue extends updated_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(updated_struct updated) {
            super();
            this.updated = updated;
            setType(updated_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRTransactionEvent() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("type", "u");
    }

    public BRTransactionEvent(int type, u_union u) {
        super();
        this.type = type;
        this.u = u;
    }

    public BRTransactionEvent(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRTransactionEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRTransactionEvent implements Structure.ByValue {

    }

    @Override
    public void read() {
        super.read();
        switch (type){
            case BRTransactionEventType.BITCOIN_TRANSACTION_UPDATED:
                u.setType(u_union.updated_struct.class);
                u.read();
                break;
        }
    }
}
