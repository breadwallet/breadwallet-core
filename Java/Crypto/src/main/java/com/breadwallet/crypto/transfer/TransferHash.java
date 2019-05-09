package com.breadwallet.crypto.transfer;

import com.breadwallet.crypto.jni.BREthereumHash;
import com.breadwallet.crypto.jni.UInt256;

import java.math.BigInteger;

public final class TransferHash {
    public final TransferType type;
    private UInt256 btcCore;
    private BREthereumHash ethCore;

    public TransferHash createBtc(UInt256 btcCore) {
        return new TransferHash(TransferType.BTC, btcCore, null);
    }

    public TransferHash createBtc(BREthereumHash ethCore) {
        return new TransferHash(TransferType.ETH, null, ethCore);
    }

    private TransferHash(TransferType type, UInt256 btcCore, BREthereumHash ethCore) {
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

        // TODO: Is this safe to do instead of calling native method?
        switch (type) {
            case BTC:
                return btcCore.u8.equals(hash.btcCore.u8);
            case ETH:
                return ethCore.u8.equals(hash.btcCore.u8);
            default:
                throw new IllegalStateException("Invalid type");
        }
    }

    @Override
    public int hashCode() {
        // TODO: Is this safe to do instead of calling native method?
        switch (type) {
            case BTC:
                return new BigInteger(btcCore.u8).intValue();
            case ETH:
                return new BigInteger(ethCore.u8).intValue();
            default:
                throw new IllegalStateException("Invalid type");
        }
    }
}
