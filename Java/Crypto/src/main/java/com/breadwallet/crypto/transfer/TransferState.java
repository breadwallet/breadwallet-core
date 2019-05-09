package com.breadwallet.crypto.transfer;

public final class TransferState {

    public final Type type;
    // TODO: How do we want these to be exposed
    /* package */ final TransferConfirmation includedConfirmation;
    /* package */ final String failedReason;

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

    private TransferState(Type type, TransferConfirmation includedConfirmation, String failedReason) {
        this.type = type;
        this.includedConfirmation = includedConfirmation;
        this.failedReason = failedReason;
    }
}
