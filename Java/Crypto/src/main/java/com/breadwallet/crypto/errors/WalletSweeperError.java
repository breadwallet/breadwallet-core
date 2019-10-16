/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.errors;

public abstract class WalletSweeperError extends Exception {

    /* package */
    WalletSweeperError() {
        super();
    }

    /* package */
    WalletSweeperError(String message) {
        super(message);
    }

    /* package */
    WalletSweeperError(Throwable throwable) {
        super(throwable);
    }
}
