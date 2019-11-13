/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/14/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.Collections;
import java.util.List;

public class TransactionList {

    // fields

    @JsonProperty("_embedded")
    private Embedded embedded;

    // getters

    public List<Transaction> getTransactions() {
        return embedded == null ? Collections.emptyList() : embedded.transactions;
    }

    // internals

    private static class Embedded {
        List<Transaction> transactions;
    }
}
