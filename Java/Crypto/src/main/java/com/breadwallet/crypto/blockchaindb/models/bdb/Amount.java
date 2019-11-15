/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/14/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

import static com.google.common.base.Preconditions.checkNotNull;

public class Amount {

    // creators

    // TODO(fix): require the currency id
    public static Amount create(String amountValue) {
        return new Amount(
                null,
                amountValue
        );
    }

    @JsonCreator
    public static Amount create(@JsonProperty("currency_id") String currencyId,
                                @JsonProperty("amount") String amount) {
        return new Amount(
                checkNotNull(currencyId),
                checkNotNull(amount)
        );
    }

    // fields

    private final String currencyId;
    private final String amount;

    private Amount(String currencyId,
                   String amount) {
        this.currencyId = currencyId;
        this.amount= amount;
    }

    // getters

    @JsonProperty("currency_id")
    public String getCurrencyId() {
        return currencyId;
    }

    @JsonProperty("amount")
    public String getAmount() {
        return amount;
    }
}
