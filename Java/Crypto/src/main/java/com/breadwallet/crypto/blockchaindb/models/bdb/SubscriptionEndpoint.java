/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

public class SubscriptionEndpoint {

    // creators

    public static SubscriptionEndpoint create(String kind, String environment, String value) {
        SubscriptionEndpoint endpoint = new SubscriptionEndpoint();
        endpoint.kind = kind;
        endpoint.environment = environment;
        endpoint.value = value;
        return endpoint;
    }

    // fields

    private String kind;
    private String environment;
    private String value;

    // getters

    public String getKind() {
        return kind;
    }

    public String getEnvironment() {
        return environment;
    }

    public String getValue() {
        return value;
    }
}
