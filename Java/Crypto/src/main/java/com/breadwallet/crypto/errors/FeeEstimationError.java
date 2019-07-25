package com.breadwallet.crypto.errors;

public abstract class FeeEstimationError extends Exception {

    public FeeEstimationError() {
        super();
    }

    public FeeEstimationError(String message) {
        super(message);
    }
}
