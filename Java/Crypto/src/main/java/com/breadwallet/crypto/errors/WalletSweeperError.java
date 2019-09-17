/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
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
