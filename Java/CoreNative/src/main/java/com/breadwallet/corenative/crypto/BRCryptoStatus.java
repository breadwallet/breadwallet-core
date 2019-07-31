package com.breadwallet.corenative.crypto;

public interface BRCryptoStatus {

    int CRYPTO_SUCCESS                              = 0;
    int CRYPTO_ERROR_UNKNOWN                        = 1;

    // Reference access
    int CRYPTO_ERROR_UNKNOWN_NODE                   = 10000;
    int CRYPTO_ERROR_UNKNOWN_TRANSFER               = 10001;
    int CRYPTO_ERROR_UNKNOWN_ACCOUNT                = 10002;
    int CRYPTO_ERROR_UNKNOWN_WALLET                 = 10003;
    int CRYPTO_ERROR_UNKNOWN_BLOCK                  = 10004;
    int CRYPTO_ERROR_UNKNOWN_LISTENER               = 10005;

    // Node
    int CRYPTO_ERROR_NODE_NOT_CONNECTED             = 20000;

    // Transfer
    int CRYPTO_ERROR_TRANSFER_HASH_MISMATCH         = 30000;
    int CRYPTO_ERROR_TRANSFER_SUBMISSION            = 30001;

    // Numeric
    int CRYPTO_ERROR_NUMERIC_PARSE                  = 40000;
}
