/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import static com.google.common.base.Preconditions.checkNotNull;

@JsonIgnoreProperties(value = {
        "is_supported",
        "sale_address",
        "contract_info",
        "colors",
        "type",
        "currency_id",
        "alternate_names"
})
public class EthToken {

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

    public String getAddress() {
        return address;
    }

    public String getSymbol() {
        return symbol;
    }

    public String getName() {
        return name;
    }

    public String getDescription() {
        return String.format("Token for %s", getSymbol());
    }

    public UnsignedInteger getDecimals() {
        return decimals;
    }

    // TODO(fix): defaultGasLimit and defaultGasPrice are not present in the JSON response

    public Optional<String> getDefaultGasLimit() {
        return Optional.absent();
    }

    public Optional<String> getDefaultGasPrice() {
        return Optional.absent();
    }
}
