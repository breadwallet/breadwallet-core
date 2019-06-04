package com.breadwallet.crypto.libcrypto.bitcoin;

import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.Union;
import java.util.Arrays;
import java.util.List;

public class BRWalletEvent extends Structure {

    public int type;
    public u_union u;
    public static class u_union extends Union {

        public balance_struct balance;
        public submitted_struct submitted;

        public static class balance_struct extends Structure {

            public long satoshi;

            public balance_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("satoshi");
            }

            public balance_struct(long satoshi) {
                super();
                this.satoshi = satoshi;
            }

            public balance_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends balance_struct implements Structure.ByReference {

            }

            public static class ByValue extends balance_struct implements Structure.ByValue {

            }
        }

        public static class submitted_struct extends Structure {

            public BRTransaction.ByReference transaction;
            public int error;

            public submitted_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("transaction", "error");
            }

            public submitted_struct(BRTransaction.ByReference transaction, int error) {
                super();
                this.transaction = transaction;
                this.error = error;
            }

            public submitted_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends submitted_struct implements Structure.ByReference {

            }

            public static class ByValue extends submitted_struct implements Structure.ByValue {

            }
        }

        public u_union() {
            super();
        }

        public u_union(balance_struct balance) {
            super();
            this.balance = balance;
            setType(balance_struct.class);
        }

        public u_union(submitted_struct submitted) {
            super();
            this.submitted = submitted;
            setType(submitted_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRWalletEvent() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("type", "u");
    }

    public BRWalletEvent(int type, u_union u) {
        super();
        this.type = type;
        this.u = u;
    }

    public BRWalletEvent(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRWalletEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRWalletEvent implements Structure.ByValue {

    }

    @Override
    public void read() {
        super.read();
        switch (type){
            case BRWalletEventType.BITCOIN_WALLET_BALANCE_UPDATED:
                u.setType(u_union.balance_struct.class);
                u.read();
                break;
            case BRWalletEventType.BITCOIN_WALLET_TRANSACTION_SUBMITTED:
                u.setType(u_union.submitted_struct.class);
                u.read();
                break;
        }
    }
}
