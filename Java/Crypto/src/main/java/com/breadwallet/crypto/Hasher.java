package com.breadwallet.crypto;

public interface Hasher {

    enum Algorithm {
        SHA1,
        SHA224,
        SHA256,
        SHA256_2,
        SHA384,
        SHA512,
        SHA3,
        RMD160,
        HASH160,
        KECCAK256,
        MD5
    }

    static Hasher createForAlgorithm(Algorithm algorithm) {
        return CryptoApi.getProvider().hasherProvider().createHasherForAlgorithm(algorithm);
    }

    byte[] hash(byte[] data);
}
