/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.errors;

public class QueryJsonParseError extends QueryError {

    public QueryJsonParseError() {
        super();
    }

    public QueryJsonParseError(String message) {
        super(message);
    }
}
