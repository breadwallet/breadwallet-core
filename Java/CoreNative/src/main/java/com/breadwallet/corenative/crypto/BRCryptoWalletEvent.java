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

public class BRCryptoWalletEvent extends Structure {

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

            public state_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("oldStateEnum", "newStateEnum");
            }

            public state_struct(int oldState, int newState) {
                super();
                this.oldStateEnum = oldState;
                this.newStateEnum = newState;
            }

            public state_struct(Pointer peer) {
                super(peer);
            }

            public BRCryptoWalletState oldState() {
                return BRCryptoWalletState.fromCore(oldStateEnum);
            }

            public BRCryptoWalletState newState() {
                return BRCryptoWalletState.fromCore(newStateEnum);
            }

            public static class ByReference extends state_struct implements Structure.ByReference {

            }

            public static class ByValue extends state_struct implements Structure.ByValue {

            }
        }

        public static class transfer_struct extends Structure {

            public BRCryptoTransfer value;

            public transfer_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("value");
            }

            public transfer_struct(BRCryptoTransfer value) {
                super();
                this.value = value;
            }

            public transfer_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends transfer_struct implements Structure.ByReference {

            }

            public static class ByValue extends transfer_struct implements Structure.ByValue {

            }
        }

        public static class balanceUpdated_struct extends Structure {

            public BRCryptoAmount amount;

            public balanceUpdated_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("amount");
            }

            public balanceUpdated_struct(BRCryptoAmount amount) {
                super();
                this.amount = amount;
            }

            public balanceUpdated_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends balanceUpdated_struct implements Structure.ByReference {

            }

            public static class ByValue extends balanceUpdated_struct implements Structure.ByValue {

            }
        }

        public static class feeBasisUpdated_struct extends Structure {

            public BRCryptoFeeBasis basis;

            public feeBasisUpdated_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("basis");
            }

            public feeBasisUpdated_struct(BRCryptoFeeBasis basis) {
                super();
                this.basis = basis;
            }

            public feeBasisUpdated_struct(Pointer peer) {
                super(peer);
            }

            public static class ByReference extends feeBasisUpdated_struct implements Structure.ByReference {

            }

            public static class ByValue extends feeBasisUpdated_struct implements Structure.ByValue {

            }
        }

        public static class feeBasisEstimated_struct extends Structure {

            public int statusEnum;
            public Pointer cookie;
            public BRCryptoFeeBasis basis;

            public feeBasisEstimated_struct() {
                super();
            }

            protected List<String> getFieldOrder() {
                return Arrays.asList("statusEnum", "cookie", "basis");
            }

            public feeBasisEstimated_struct(int status, Pointer cookie, BRCryptoFeeBasis basis) {
                super();
                this.statusEnum = status;
                this.cookie = cookie;
                this.basis = basis;
            }

            public feeBasisEstimated_struct(Pointer peer) {
                super(peer);
            }

            public BRCryptoStatus status() {
                return BRCryptoStatus.fromCore(statusEnum);
            }

            public static class ByReference extends feeBasisEstimated_struct implements Structure.ByReference {

            }

            public static class ByValue extends feeBasisEstimated_struct implements Structure.ByValue {

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

        public u_union(transfer_struct transfer) {
            super();
            this.transfer = transfer;
            setType(transfer_struct.class);
        }

        public u_union(balanceUpdated_struct balanceUpdated) {
            super();
            this.balanceUpdated = balanceUpdated;
            setType(balanceUpdated_struct.class);
        }

        public u_union(feeBasisUpdated_struct feeBasisUpdated) {
            super();
            this.feeBasisUpdated = feeBasisUpdated;
            setType(feeBasisUpdated_struct.class);
        }

        public u_union(feeBasisEstimated_struct feeBasisEstimated) {
            super();
            this.feeBasisEstimated = feeBasisEstimated;
            setType(feeBasisEstimated_struct.class);
        }

        public u_union(Pointer peer) {
            super(peer);
        }

        public static class ByReference extends u_union implements Structure.ByReference {

        }

        public static class ByValue extends u_union implements Structure.ByValue {

        }
    }

    public BRCryptoWalletEvent() {
        super();
    }

    public BRCryptoWalletEventType type() {
        return BRCryptoWalletEventType.fromCore(typeEnum);
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("typeEnum", "u");
    }

    public BRCryptoWalletEvent(int type, u_union u) {
        super();
        this.typeEnum = type;
        this.u = u;
    }

    @Override
    public void read() {
        super.read();
        switch (type()){
            case CRYPTO_WALLET_EVENT_CHANGED:
                u.setType(u_union.state_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_EVENT_BALANCE_UPDATED:
                u.setType(u_union.balanceUpdated_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_EVENT_TRANSFER_ADDED:
            case CRYPTO_WALLET_EVENT_TRANSFER_DELETED:
            case CRYPTO_WALLET_EVENT_TRANSFER_CHANGED:
            case CRYPTO_WALLET_EVENT_TRANSFER_SUBMITTED:
                u.setType(u_union.transfer_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_UPDATED:
                u.setType(u_union.feeBasisUpdated_struct.class);
                u.read();
                break;
            case CRYPTO_WALLET_EVENT_FEE_BASIS_ESTIMATED:
                u.setType(u_union.feeBasisEstimated_struct.class);
                u.read();
                break;
        }
    }

    public BRCryptoWalletEvent(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoWalletEvent implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoWalletEvent implements Structure.ByValue {

    }
}
