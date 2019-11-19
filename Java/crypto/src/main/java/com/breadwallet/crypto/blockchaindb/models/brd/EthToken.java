/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthToken {

    // creators

    @JsonCreator
    public static EthToken create(@JsonProperty("contract_address") String address,
                                  @JsonProperty("code") String symbol,
                                  @JsonProperty("name") String name,
                                  @JsonProperty("scale") UnsignedInteger decimals) {
        return new EthToken(
                checkNotNull(address),
                checkNotNull(symbol),
                checkNotNull(name),
                checkNotNull(decimals)
        );
    }

    // fields

    private final String address;
    private final String symbol;
    private final String name;
    private final UnsignedInteger decimals;

    private EthToken(String address,
                     String symbol,
                     String name,
                     UnsignedInteger decimals) {
        this.address = address;
        this.symbol = symbol;
        this.name = name;
        this.decimals = decimals;
    }

    // getters

    @JsonProperty("contract_address")
    public String getAddress() {
        return address;
    }

    @JsonProperty("code")
    public String getSymbol() {
        return symbol;
    }

    @JsonProperty("name")
    public String getName() {
        return name;
    }

    @JsonProperty("scale")
    public UnsignedInteger getDecimals() {
        return decimals;
    }

    @JsonIgnore
    public String getDescription() {
        return String.format("Token for %s", getSymbol());
    }

    // TODO(fix): defaultGasLimit and defaultGasPrice are not present in the JSON response

    @JsonIgnore
    public Optional<String> getDefaultGasLimit() {
        return Optional.absent();
    }

    @JsonIgnore
    public Optional<String> getDefaultGasPrice() {
        return Optional.absent();
    }
}
