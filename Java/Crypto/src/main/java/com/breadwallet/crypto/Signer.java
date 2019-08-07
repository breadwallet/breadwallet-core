package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Signer {

    enum Algorithm {
        BASIC,
        COMPACT
    }

    static Signer createForAlgorithm(Algorithm algorithm) {
        return CryptoApi.getProvider().signerProvider().createSignerForAlgorithm(algorithm);
    }

    byte[] sign(byte[] data, Key key);

    Optional<? extends Key> recover(byte[] data, byte[] signature);
}
