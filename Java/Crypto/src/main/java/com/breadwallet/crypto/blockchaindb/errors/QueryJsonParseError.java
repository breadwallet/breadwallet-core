/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.errors;

// JSON parse failed, generically
public class QueryJsonParseError extends QueryError {

    public QueryJsonParseError() {
        super();
    }

    public QueryJsonParseError(String message) {
        super(message);
    }
}
