package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoTransferAttribute;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

import javax.annotation.Nullable;

public class TransferAttribute implements com.breadwallet.crypto.TransferAttribute {

    /* package */
    static TransferAttribute create(BRCryptoTransferAttribute core) {
        TransferAttribute attribute = new TransferAttribute(core);
        ReferenceCleaner.register(attribute, core::give);
        return attribute;
    }

    static TransferAttribute from(com.breadwallet.crypto.TransferAttribute attribute) {
        if (attribute == null) {
            return null;
        }

        if (attribute instanceof TransferAttribute) {
            return (TransferAttribute) attribute;
        }

        throw new IllegalArgumentException("Unsupported TransferAttribute instance");
    }

    private final BRCryptoTransferAttribute core;

//    private final int value;
    private final Supplier<String> keySupplier;
    private final Supplier<Boolean> isRequiredSupplier;

    private TransferAttribute(BRCryptoTransferAttribute core) {
        this.core = core;

        this.keySupplier = Suppliers.memoize(() -> core.getKey());
        this.isRequiredSupplier = Suppliers.memoize(() -> core.isRequired());
    }

    @Override
    public String getKey() {
        return keySupplier.get();
    }

    @Override
    public Optional<String> getValue() {
        return core.getValue();
    }

    @Override
    public void setValue(@Nullable String value) {
        core.setValue(value);
    }

    @Override
    public boolean isRequired() {
        return isRequiredSupplier.get();
    }

    public TransferAttribute copy () {
        return TransferAttribute.create (core.copy());
    }

    /* package */
    BRCryptoTransferAttribute getCoreBRCryptoTransferAttribute() {
        return core;
    }

    public boolean equalsTransferAttribute (TransferAttribute that) {
        return this.core == that.core || this.getKey().equals(that.getKey());
    }

    @Override
    public boolean equals(Object that) {
        return this == that
                || (that instanceof TransferAttribute && equalsTransferAttribute((TransferAttribute) that))
                || (that instanceof com.breadwallet.crypto.TransferAttribute && this.getKey().equals(((com.breadwallet.crypto.TransferAttribute) that).getKey()));
    }

    @Override
    public int hashCode() {
        return getKey().hashCode();
    }
}
