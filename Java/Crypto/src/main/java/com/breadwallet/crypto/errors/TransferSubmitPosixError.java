/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 9/18/19.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.errors;

import java.util.Locale;
import java.util.Objects;

public final class TransferSubmitPosixError extends TransferSubmitError {

    private final int errnum;

    public TransferSubmitPosixError(int errnum, String message) {
        super(String.format(Locale.ROOT, "Posix (%d: %s)", errnum, message));
        this.errnum = errnum;
    }

    public int getErrnum() {
        return errnum;
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof TransferSubmitPosixError)) {
            return false;
        }

        TransferSubmitPosixError that = (TransferSubmitPosixError) object;
        return errnum == that.errnum;
    }

    @Override
    public int hashCode() {
        return Objects.hash(errnum);
    }
}
