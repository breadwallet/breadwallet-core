package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import java.util.Locale;
import java.util.Objects;

public final class WalletManagerDisconnectReason {

    // create constant values, where possible
    private static final WalletManagerDisconnectReason REQUESTED_REASON = new WalletManagerDisconnectReason(Type.REQUESTED);
    private static final WalletManagerDisconnectReason UNKNOWN_REASON = new WalletManagerDisconnectReason(Type.UNKNOWN);

    public static WalletManagerDisconnectReason REQUESTED() {
        return REQUESTED_REASON;
    }

    public static WalletManagerDisconnectReason UNKNOWN() {
        return UNKNOWN_REASON;
    }

    public static WalletManagerDisconnectReason POSIX(int errNum, @Nullable String errMessage) {
        return new WalletManagerDisconnectReason(Type.POSIX, errNum, errMessage);
    }

    public enum Type { REQUESTED, UNKNOWN, POSIX }

    private final Type type;

    @Nullable
    private final Integer posixErrnum;

    @Nullable
    private final String message;

    private WalletManagerDisconnectReason(Type type) {
        this(type, null, null);
    }

    private WalletManagerDisconnectReason(Type type,
                                          @Nullable Integer posixErrnum,
                                          @Nullable String message) {
        this.type = type;
        this.posixErrnum = posixErrnum;
        this.message = message;
    }

    public Type getType() {
        return type;
    }

    public Optional<Integer> getPosixErrnum() {
        return Optional.fromNullable(posixErrnum);
    }

    public Optional<String> getMessage() {
        return Optional.fromNullable(message);
    }

    @Override
    public String toString() {
        switch (type) {
            case REQUESTED:
                return "Requested";
            case UNKNOWN:
                return "Unknown";
            case POSIX:
                return String.format(Locale.ROOT, "Posix (%d: %s)", posixErrnum, message);
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
