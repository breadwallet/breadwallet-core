/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.primitives.UnsignedLong;

import static com.google.common.base.Preconditions.checkNotNull;

public class BlockchainFee {

    // creators

    public static BlockchainFee create(String amount,
                                       String tier,
                                       UnsignedLong confirmationTimeInMilliseconds) {
        return create(
                Amount.create(amount),
                tier,
                confirmationTimeInMilliseconds
        );
    }

    @JsonCreator
    public static BlockchainFee create(@JsonProperty("fee") Amount fee,
                                       @JsonProperty("tier") String tier,
                                       @JsonProperty("estimated_confirmation_in") UnsignedLong confirmationTimeInMilliseconds) {
        return new BlockchainFee(
                checkNotNull(fee),
                checkNotNull(tier),
                checkNotNull(confirmationTimeInMilliseconds)
        );
    }

    // fields

    private final Amount fee;
    private final String tier;
    private final UnsignedLong confirmationTimeInMilliseconds;

    private BlockchainFee(Amount fee,
                         String tier,
                         UnsignedLong confirmationTimeInMilliseconds) {
        this.fee = fee;
        this.tier = tier;
        this.confirmationTimeInMilliseconds = confirmationTimeInMilliseconds;
    }

    // getters

    @JsonProperty("fee")
    public Amount getFee() {
        return fee;
    }

    @JsonProperty("tier")
    public String getTier() {
        return tier;
    }

    @JsonProperty("estimated_confirmation_in")
    public UnsignedLong getConfirmationTimeInMilliseconds() {
        return confirmationTimeInMilliseconds;
    }

    @JsonIgnore
    public String getAmount() {
        return fee.getAmount();
    }
}
