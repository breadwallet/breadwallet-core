/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.CryptoLibrary;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.Date;
import java.util.concurrent.TimeUnit;

public class BRCryptoAccount extends PointerType implements CoreBRCryptoAccount {

    public BRCryptoAccount(Pointer address) {
        super(address);
    }

    public BRCryptoAccount() {
        super();
    }

    @Override
    public Date getTimestamp() {
        return new Date(TimeUnit.SECONDS.toMillis(CryptoLibrary.INSTANCE.cryptoAccountGetTimestamp(this)));
    }

    @Override
    public String getFilesystemIdentifier() {
        Pointer ptr = CryptoLibrary.INSTANCE.cryptoAccountGetFileSystemIdentifier(this);
        try {
            return ptr.getString(0, "UTF-8");
        } finally {
            Native.free(Pointer.nativeValue(ptr));
        }
    }

    @Override
    public byte[] serialize() {
        SizeTByReference bytesCount = new SizeTByReference();
        Pointer serializationPtr = CryptoLibrary.INSTANCE.cryptoAccountSerialize(this, bytesCount);
        try {
            return serializationPtr.getByteArray(0, UnsignedInts.checkedCast(bytesCount.getValue().longValue()));
        } finally {
            Native.free(Pointer.nativeValue(serializationPtr));
        }
    }

    @Override
    public boolean validate(byte[] serialization) {
        return BRCryptoBoolean.CRYPTO_TRUE == CryptoLibrary.INSTANCE.cryptoAccountValidateSerialization(this,
                serialization, new SizeT(serialization.length));
    }

    @Override
    public BRCryptoAccount asBRCryptoAccount() {
        return this;
    }
}
