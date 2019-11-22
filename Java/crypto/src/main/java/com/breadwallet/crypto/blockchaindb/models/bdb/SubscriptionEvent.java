/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.primitives.UnsignedInteger;
import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class SubscriptionEvent {

    // creators

    @JsonCreator
    public static SubscriptionEvent create(@JsonProperty("name") String name,
                                           @JsonProperty("confirmations") List<UnsignedInteger> confirmations) {
        return new SubscriptionEvent(
                checkNotNull(name),
                checkNotNull(confirmations)
        );
    }

    // fields

    private final String name;
    private final List<UnsignedInteger> confirmations;

    private SubscriptionEvent(String name,
                              List<UnsignedInteger> confirmations) {
        this.name = name;
        this.confirmations = confirmations;
    }

    // getters

    @JsonProperty("name")
    public String getName() {
        return name;
    }

    @JsonProperty("confirmations")
    public List<UnsignedInteger> getConfirmations() {
        return confirmations;
    }
}
