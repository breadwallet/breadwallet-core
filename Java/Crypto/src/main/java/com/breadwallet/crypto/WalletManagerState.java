/*
 * WalletManagerState
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import java.util.Objects;

public final class WalletManagerState {

    // create constant values, where possible
    private static final WalletManagerState CREATED_STATE = new WalletManagerState(Type.CREATED);
    private static final WalletManagerState CONNECTED_STATE = new WalletManagerState(Type.CONNECTED);
    private static final WalletManagerState SYNCING_STATE = new WalletManagerState(Type.SYNCING);
    private static final WalletManagerState DELETED_STATE = new WalletManagerState(Type.DELETED);

    public static WalletManagerState CREATED() {
        return CREATED_STATE;
    }

    public static WalletManagerState DISCONNECTED(WalletManagerDisconnectReason reason) {
        return new WalletManagerState(Type.DISCONNECTED, reason);
    }

    public static WalletManagerState CONNECTED() {
        return CONNECTED_STATE;
    }

    public static WalletManagerState SYNCING() {
        return SYNCING_STATE;
    }

    public static WalletManagerState DELETED() {
        return DELETED_STATE;
    }

    public enum Type { CREATED, DISCONNECTED, CONNECTED, SYNCING, DELETED }

    private final Type type;

    @Nullable
    private final WalletManagerDisconnectReason reason;

    private WalletManagerState(Type type) {
        this(type, null);
    }

    private WalletManagerState(Type type, @Nullable WalletManagerDisconnectReason reason) {
        this.type = type;
        this.reason = reason;
    }

    public Type getType() {
        return type;
    }

    public Optional<WalletManagerDisconnectReason> getDisconnectReason() {
        return Optional.fromNullable(reason);
    }

    @Override
    public String toString() {
        switch (type) {
            case DELETED:
                return "Deleted";
            case CREATED:
                return "Created";
            case SYNCING:
                return "Syncing";
            case CONNECTED:
                return "Connected";
            case DISCONNECTED:
                return String.format("Disconnected (%s)", reason);
            default:
                return super.toString();
        }
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof WalletManagerState)) {
            return false;
        }

        WalletManagerState that = (WalletManagerState) object;
        return type == that.type &&
                Objects.equals(reason, that.reason);
    }

    @Override
    public int hashCode() {
        return Objects.hash(type, reason);
    }
}
