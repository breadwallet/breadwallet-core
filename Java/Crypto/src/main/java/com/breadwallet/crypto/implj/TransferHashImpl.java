/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.implj;

import com.breadwallet.crypto.TransferHash;
import com.breadwallet.crypto.libcrypto.ethereum.BREthereumHash;
import com.breadwallet.crypto.libcrypto.support.UInt256;
import com.google.common.io.BaseEncoding;

import java.math.BigInteger;
import java.util.Arrays;
import java.util.Objects;

import static com.google.common.base.Preconditions.checkArgument;

/* package */
final class TransferHashImpl implements TransferHash {

    private static final BaseEncoding BASE16_ENCODER = BaseEncoding.base16().lowerCase();

    private final CurrencyTransferHash impl;

    /* package */
    static TransferHashImpl createBtc(UInt256 btcCore) {
        return new TransferHashImpl(new BitcoinTransferHash(btcCore));
    }

    /* package */
    static TransferHashImpl createEth(BREthereumHash ethCore) {
        return new TransferHashImpl(new EthereumTransferHash(ethCore));
    }

    private TransferHashImpl(CurrencyTransferHash impl) {
        this.impl = impl;
    }

    private interface CurrencyTransferHash {
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }

        if (!(object instanceof TransferHashImpl)) {
            return false;
        }

        TransferHashImpl that = (TransferHashImpl) object;
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

    // TODO(fix): Do the equals, hashcode and toString implementations yield the same result as calling native method?
    private static class BitcoinTransferHash implements CurrencyTransferHash {

        private final byte[] hash;

        BitcoinTransferHash(UInt256 core) {
            checkArgument(core != null);
            int length = core.u8.length;
            hash = new byte[length];
            for (int i = 0; i < length; i++) {
                hash[i] = core.u8[length - i - 1];
            }
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof BitcoinTransferHash)) {
                return false;
            }

            BitcoinTransferHash bitcoin = (BitcoinTransferHash) object;
            return Arrays.equals(hash, bitcoin.hash);
        }

        @Override
        public int hashCode() {
            return new BigInteger(hash).intValue();
        }

        @Override
        public String toString() {
            return "0x" + BASE16_ENCODER.encode(hash);
        }
    }

    private static class EthereumTransferHash implements CurrencyTransferHash {

        private final BREthereumHash core;

        EthereumTransferHash(BREthereumHash core) {
            checkArgument(core != null);
            this.core = core;
        }

        @Override
        public boolean equals(Object object) {
            if (this == object) {
                return true;
            }

            if (!(object instanceof EthereumTransferHash)) {
                return false;
            }

            EthereumTransferHash ethereum = (EthereumTransferHash) object;
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
