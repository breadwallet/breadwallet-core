package com.breadwallet.crypto;

import com.breadwallet.crypto.jni.ethereum.BREthereumHash;
import com.breadwallet.crypto.jni.support.UInt256;
import com.google.common.io.BaseEncoding;

import java.math.BigInteger;
import java.util.Arrays;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;

public final class TransferHash {

    private static final BaseEncoding BASE16_ENCODER = BaseEncoding.base16().lowerCase();

    private final Impl impl;

    /* package */
    static TransferHash createBtc(UInt256 btcCore) {
        return new TransferHash(new Bitcoin(btcCore));
    }

    /* package */
    static TransferHash createEth(BREthereumHash ethCore) {
        return new TransferHash(new Ethereum(ethCore));
    }

    private TransferHash(Impl impl) {
        this.impl = impl;
    }

    private interface Impl {
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
        return impl.equals(that.impl);
    }

    @Override
    public int hashCode() {
        return Objects.hash(impl);
    }

    @Override
    public String toString() {
        return impl.toString();
    }

    // TODO: Do the equals, hashcode and toString implementations yield the same result as calling native method?

    private static class Bitcoin implements Impl {

        private UInt256 core;

        Bitcoin(UInt256 core) {
            checkArgument(core != null);
            this.core = core;
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof Bitcoin)) {
                return false;
            }

            Bitcoin bitcoin = (Bitcoin) object;
            return Arrays.equals(core.u8, bitcoin.core.u8);
        }

        @Override
        public int hashCode() {
            return new BigInteger(core.u8).intValue();
        }

        @Override
        public String toString() {
            return "0x" + BASE16_ENCODER.encode(core.u8);
        }
    }

    private static class Ethereum implements Impl {

        private BREthereumHash core;

        Ethereum(BREthereumHash core) {
            checkArgument(core != null);
            this.core = core;
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof Ethereum)) {
                return false;
            }

            Ethereum ethereum = (Ethereum) object;
            return Arrays.equals(core.u8, ethereum.core.u8);
        }

        @Override
        public int hashCode() {
            return new BigInteger(core.u8).intValue();
        }

        @Override
        public String toString() {
            return "0x" + BASE16_ENCODER.encode(core.u8);
        }
    }
}
