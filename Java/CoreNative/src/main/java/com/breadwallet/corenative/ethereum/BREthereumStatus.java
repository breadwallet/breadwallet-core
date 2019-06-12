package com.breadwallet.corenative.ethereum;

public interface BREthereumStatus {

    int SUCCESS = 0;
    int ERROR_UNKNOWN_NODE = 1;
    int ERROR_UNKNOWN_TRANSACTION = 2;
    int ERROR_UNKNOWN_ACCOUNT = 3;
    int ERROR_UNKNOWN_WALLET = 4;
    int ERROR_UNKNOWN_BLOCK = 5;
    int ERROR_UNKNOWN_LISTENER = 6;
    int ERROR_NODE_NOT_CONNECTED = 7;
    int ERROR_TRANSACTION_HASH_MISMATCH = 8;
    int ERROR_TRANSACTION_SUBMISSION = 9;
    int ERROR_NUMERIC_PARS = 10;
}
