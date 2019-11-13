/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/14/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;

public class Amount {

    // creators

    public static Amount create(String amountValue) {
        Amount amount = new Amount();
        amount.currencyId = null;
        amount.amount = amountValue;
        return amount;
    }

    // fields

    @JsonProperty("currency_id")
    private String currencyId;

    private String amount;

    // getters

    public String getCurrencyId() {
        return currencyId;
    }

    public String getAmount() {
        return amount;
    }
}
