package com.breadwallet.crypto.errors;

public abstract class WalletSweeperError extends Exception {

    public WalletSweeperError() {
        super();
    }

    public WalletSweeperError(String message) {
        super(message);
    }

    public WalletSweeperError(String message, Throwable throwable) {
        super(message, throwable);
    }

    public WalletSweeperError(Throwable throwable) {
        super(throwable);
    }
}
