/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.primitives.UnsignedLong;

public class BlockchainFee {

    // creators

    public static BlockchainFee create(String amount, String tier, UnsignedLong confirmationTimeInMilliseconds) {
        BlockchainFee fee = new BlockchainFee();
        fee.fee = Amount.create(amount);
        fee.estimatedConfirmationIn = confirmationTimeInMilliseconds;
        return fee;
    }

    // fields

    private Amount fee;

    private String tier;

    @JsonProperty("estimated_confirmation_in")
    private UnsignedLong estimatedConfirmationIn;

    // getters

    public Amount getFee() {
        return fee;
    }

    public String getTier() {
        return tier;
    }

    public String getAmount() {
        return fee.getAmount();
    }

    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return estimatedConfirmationIn;
    }
}
