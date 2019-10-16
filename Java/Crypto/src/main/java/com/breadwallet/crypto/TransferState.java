/*
 * TransferState
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 1/22/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto;

import android.support.annotation.Nullable;

import com.breadwallet.crypto.errors.TransferSubmitError;
import com.google.common.base.Optional;

import java.util.Objects;

public final class TransferState {

    // create constant values, where possible
    private static final TransferState CREATED_STATE = new TransferState(Type.CREATED, null, null);
    private static final TransferState SIGNED_STATE = new TransferState(Type.SIGNED, null, null);
    private static final TransferState SUBMITTED_STATE = new TransferState(Type.SUBMITTED, null, null);
    private static final TransferState PENDING_STATE = new TransferState(Type.PENDING, null, null);
    private static final TransferState DELETED_STATE = new TransferState(Type.DELETED, null, null);

    public static TransferState CREATED() {
        return CREATED_STATE;
    }

    public static TransferState SIGNED() {
        return SIGNED_STATE;
    }

    public static TransferState SUBMITTED() {
        return SUBMITTED_STATE;
    }

    public static TransferState PENDING() {
        return PENDING_STATE;
    }

    public static TransferState INCLUDED(TransferConfirmation confirmation) {
        return new TransferState(Type.INCLUDED, confirmation, null);
    }

    public static TransferState FAILED(TransferSubmitError error) {
        return new TransferState(Type.FAILED, null, error);
    }

    public static TransferState DELETED() {
        return DELETED_STATE;
    }

    public enum Type { CREATED, SIGNED, SUBMITTED, PENDING, INCLUDED, FAILED, DELETED }

    private final Type type;

    @Nullable
    private final TransferConfirmation includedConfirmation;
    @Nullable
    private final TransferSubmitError failedError;

    private TransferState(Type type, @Nullable TransferConfirmation includedConfirmation, @Nullable TransferSubmitError failedError) {
        this.type = type;
        this.includedConfirmation = includedConfirmation;
        this.failedError = failedError;
    }

    public Type getType() {
        return type;
    }

    public Optional<TransferConfirmation> getIncludedConfirmation() {
        return Optional.fromNullable(includedConfirmation);
    }

    public Optional<TransferSubmitError> getFailedError() {
        return Optional.fromNullable(failedError);
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

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof TransferState)) {
            return false;
        }

        TransferState that = (TransferState) object;
        return type == that.type &&
                Objects.equals(includedConfirmation, that.includedConfirmation) &&
                Objects.equals(failedError, that.failedError);
    }

    @Override
    public int hashCode() {
        return Objects.hash(type, includedConfirmation, failedError);
    }
}
