package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

public final class TransferState {

    public static TransferState CREATED() {
        return new TransferState(Type.CREATED, null, null);
    }

    public static TransferState SIGNED() {
        return new TransferState(Type.SIGNED, null, null);
    }

    public static TransferState SUBMITTED() {
        return new TransferState(Type.SUBMITTED, null, null);
    }

    public static TransferState PENDING() {
        return new TransferState(Type.PENDING, null, null);
    }

    public static TransferState INCLUDED(TransferConfirmation confirmation) {
        return new TransferState(Type.INCLUDED, confirmation, null);
    }

    public static TransferState FAILED(String reason) {
        return new TransferState(Type.FAILED, null, reason);
    }

    public static TransferState DELETED() {
        return new TransferState(Type.DELETED, null, null);
    }

    public enum Type { CREATED, SIGNED, SUBMITTED, PENDING, INCLUDED, FAILED, DELETED }

    private final Type type;

    @Nullable
    private final TransferConfirmation includedConfirmation;
    @Nullable
    private final String failedReason;

    private TransferState(Type type, @Nullable TransferConfirmation includedConfirmation, @Nullable String failedReason) {
        this.type = type;
        this.includedConfirmation = includedConfirmation;
        this.failedReason = failedReason;
    }

    public Type getType() {
        return type;
    }

    public Optional<TransferConfirmation> getIncludedConfirmation() {
        return Optional.fromNullable(includedConfirmation);
    }

    public Optional<String> getFailedReason() {
        return Optional.fromNullable(failedReason);
    }

    @Override
    public String toString() {
        switch (type) {
            case CREATED:
                return "Created";
            case SIGNED:
                return "Signed";
            case SUBMITTED:
                return "Submitted";
            case PENDING:
                return "Pending";
            case INCLUDED:
                return "Included";
            case FAILED:
                return "Failed";
            case DELETED:
                return "Deleted";
            default:
                throw new IllegalStateException("Invalid type");
        }
    }
}
