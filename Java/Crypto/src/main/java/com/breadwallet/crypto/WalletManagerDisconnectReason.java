package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import java.util.Objects;

public final class WalletManagerDisconnectReason {

    // create constant values, where possible
    private static final WalletManagerDisconnectReason TEARDOWN_REASON = new WalletManagerDisconnectReason(Type.TEARDOWN);
    private static final WalletManagerDisconnectReason UNKNOWN_REASON = new WalletManagerDisconnectReason(Type.UNKNOWN);

    public static WalletManagerDisconnectReason TEARDOWN() {
        return TEARDOWN_REASON;
    }

    public static WalletManagerDisconnectReason UNKNOWN() {
        return UNKNOWN_REASON;
    }

    public static WalletManagerDisconnectReason POSIX(int errNum) {
        return new WalletManagerDisconnectReason(Type.POSIX, errNum);
    }

    public enum Type { TEARDOWN, UNKNOWN, POSIX }

    private final Type type;

    @Nullable
    private final Integer posixErrnum;

    private WalletManagerDisconnectReason(Type type) {
        this(type, null);
    }

    private WalletManagerDisconnectReason(Type type, @Nullable Integer posixErrnum) {
        this.type = type;
        this.posixErrnum = posixErrnum;
    }

    public Type getType() {
        return type;
    }

    public Optional<Integer> getPosixErrnum() {
        return Optional.fromNullable(posixErrnum);
    }

    @Override
    public String toString() {
        switch (type) {
            case TEARDOWN:
                return "Teardown";
            case UNKNOWN:
                return "Unknown";
            case POSIX:
                return String.format("Posix (%d)", posixErrnum);
            default:
                return super.toString();
        }
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof WalletManagerDisconnectReason)) {
            return false;
        }

        WalletManagerDisconnectReason that = (WalletManagerDisconnectReason) object;
        return type == that.type &&
                Objects.equals(posixErrnum, that.posixErrnum);
    }

    @Override
    public int hashCode() {
        return Objects.hash(type, posixErrnum);
    }
}
