/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 11/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.google.common.primitives.UnsignedInteger;
import java.util.List;

public class SubscriptionEvent {

    // creators

    public static SubscriptionEvent create(String name, List<UnsignedInteger> confirmations) {
        SubscriptionEvent event = new SubscriptionEvent();
        event.name = name;
        event.confirmations = confirmations;
        return event;
    }

    // fields

    private String name;
    private List<UnsignedInteger> confirmations;

    // getters

    public String getName() {
        return name;
    }

    public List<UnsignedInteger> getConfirmations() {
        return confirmations;
    }
}
