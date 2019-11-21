/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.errors;

import com.breadwallet.crypto.blockchaindb.errors.QueryError;

public final class WalletSweeperQueryError extends WalletSweeperError {
    public WalletSweeperQueryError(QueryError e) {
        super(e);
    }
}
