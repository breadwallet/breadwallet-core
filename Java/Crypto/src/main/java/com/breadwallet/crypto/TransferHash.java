package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.BREthereumHash;
import com.breadwallet.crypto.jni.UInt256;
import com.google.common.io.BaseEncoding;

import java.math.BigInteger;
import java.util.Arrays;

import javax.annotation.Nullable;

import static com.google.common.base.Preconditions.checkArgument;

public final class TransferHash {
    public final TransferType type;

    @Nullable
    private UInt256 btcCore;
    @Nullable
    private BREthereumHash ethCore;

    public TransferHash createBtc(UInt256 btcCore) {
        return new TransferHash(TransferType.BTC, btcCore, null);
    }

    public TransferHash createBtc(BREthereumHash ethCore) {
        return new TransferHash(TransferType.ETH, null, ethCore);
    }

    private TransferHash(TransferType type, @Nullable UInt256 btcCore, @Nullable BREthereumHash ethCore) {
        checkArgument((type == TransferType.BTC && null != btcCore) ||
                (type == TransferType.ETH && null != ethCore));

        this.type = type;
        this.btcCore = btcCore;
        this.ethCore = ethCore;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }

        if (o == null || getClass() != o.getClass()) {
            return false;
        }

        TransferHash hash = (TransferHash) o;
        if (type != ((TransferHash) o).type) {
            return false;
        }

        // TODO: Does this yield the same result as calling native method?
        switch (type) {
            case BTC:
                return Arrays.equals(btcCore.u8, hash.btcCore.u8);
            case ETH:
                return Arrays.equals(ethCore.u8, hash.btcCore.u8);
            default:
                throw new IllegalStateException("Invalid type");
        }
    }

    @Override
    public int hashCode() {
        // TODO: Does this yield the same result as calling native method?
        switch (type) {
            case BTC:
                return new BigInteger(btcCore.u8).intValue();
            case ETH:
                return new BigInteger(ethCore.u8).intValue();
            default:
                throw new IllegalStateException("Invalid type");
        }
    }

    @Override
    public String toString() {
        switch (type) {
            // TODO: Does this yield the same result as calling native method?
            case BTC:
                return BaseEncoding.base16().encode(btcCore.u8);
            case ETH:
                return BaseEncoding.base16().encode(ethCore.u8);
            default:
                throw new IllegalStateException("Invalid type");
        }
    }
}
