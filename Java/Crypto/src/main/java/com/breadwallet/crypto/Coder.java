package com.breadwallet.crypto;

import com.google.common.base.Optional;

public interface Coder {

    enum Algorithm {
        HEX,
        BASE58,
        BASE58CHECK
    }

    static Coder createForAlgorithm(Algorithm algorithm) {
        return CryptoApi.getProvider().coderPrivider().createCoderForAlgorithm(algorithm);
    }

    String encode(byte[] source);

    Optional<byte[]> decode(String source);
}
