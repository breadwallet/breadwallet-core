/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.errors;

public abstract class TransferSubmitError extends Exception {

    /* package */
    TransferSubmitError() {
        super();
    }

    /* package */
    TransferSubmitError(String message) {
        super(message);
    }
}
