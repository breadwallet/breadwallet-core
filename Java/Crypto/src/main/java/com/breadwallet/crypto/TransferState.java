package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

public final class TransferState {

    public final Type type;

    @Nullable
    private final TransferConfirmation includedConfirmation;
    @Nullable
    private final String failedReason;

    enum Type { CREATED, SIGNED, SUBMITTED, PENDING, INCLUDED, FAILED, DELETED };

    public static TransferState createCreated() {
        return new TransferState(Type.CREATED, null, null);
    }

    public static TransferState createSigned() {
        return new TransferState(Type.SIGNED, null, null);
    }

    public static TransferState createSubmitted() {
        return new TransferState(Type.SUBMITTED, null, null);
    }

    public static TransferState createPending() {
        return new TransferState(Type.PENDING, null, null);
    }

    public static TransferState createIncluded(TransferConfirmation confirmation) {
        return new TransferState(Type.INCLUDED, confirmation, null);
    }

    public static TransferState createFailed(String reason) {
        return new TransferState(Type.FAILED, null, reason);
    }

    public static TransferState createDeleted() {
        return new TransferState(Type.DELETED, null, null);
    }

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
