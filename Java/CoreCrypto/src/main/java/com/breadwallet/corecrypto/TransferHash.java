/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoHash;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import java.util.Objects;

/* package */
final class TransferHash implements com.breadwallet.crypto.TransferHash {

    /* package */
    static TransferHash create(BRCryptoHash core) {
        TransferHash hash = new TransferHash(core);
        ReferenceCleaner.register(hash, core::give);
        return hash;
    }

    private final BRCryptoHash core;

    private final int value;
    private final Supplier<String> toStringSupplier;

    private TransferHash(BRCryptoHash core) {
        this.core = core;

        this.value = core.getValue();
        this.toStringSupplier = Suppliers.memoize(core::toString);
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof TransferHash)) {
            return false;
        }

        TransferHash that = (TransferHash) object;
        return core.isIdentical(that.core);
    }

    @Override
    public int hashCode() {
        return Objects.hash(value);
    }

    @Override
    public String toString() {
        return toStringSupplier.get();
    }
}
