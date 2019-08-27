package com.breadwallet.crypto.errors;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

public class WalletSweeperQueryError extends WalletSweeperError {
    public WalletSweeperQueryError(QueryError e) {
        super(e);
    }
}
