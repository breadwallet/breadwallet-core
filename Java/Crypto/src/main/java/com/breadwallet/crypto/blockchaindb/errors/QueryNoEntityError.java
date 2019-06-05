/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.errors;

public class QueryNoEntityError extends QueryError {

    private final String id;

    public QueryNoEntityError(String id) {
        super("No entity for id " + id);
        this.id = id;
    }

    public String getUrl() {
        return id;
    }
}
