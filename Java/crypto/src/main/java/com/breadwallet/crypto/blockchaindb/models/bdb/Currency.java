/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class Currency {

    private static final String ADDRESS_INTERNAL = "__native__";

    // creators

    public static Currency create(String currencyId,
                                  String name,
                                  String code,
                                  String type,
                                  String blockchainId,
                                  @Nullable String address,
                                  Boolean verified,
                                  List<CurrencyDenomination> denominations) {
        // TODO(fix): What should the supply values be here?
        return create(
                currencyId,
                name,
                code,
                "0",
                "0",
                type,
                blockchainId,
                address == null ? ADDRESS_INTERNAL : address,
                verified,
                denominations
        );
    }

    @JsonCreator
    public static Currency create(@JsonProperty("currency_id") String currencyId,
                                  @JsonProperty("name") String name,
                                  @JsonProperty("code") String code,
                                  @JsonProperty("initial_supply") String initialSupply,
                                  @JsonProperty("total_supply") String totalSupply,
                                  @JsonProperty("type") String type,
                                  @JsonProperty("blockchain_id") String blockchainId,
                                  @JsonProperty("address") String address,
                                  @JsonProperty("verified") Boolean verified,
                                  @JsonProperty("denominations") List<CurrencyDenomination> denominations) {
        return new Currency(
                checkNotNull(currencyId),
                checkNotNull(name),
                checkNotNull(code),
                checkNotNull(initialSupply),
                checkNotNull(totalSupply),
                checkNotNull(type),
                checkNotNull(blockchainId),
                checkNotNull(address),
                checkNotNull(verified),
                checkNotNull(denominations)
        );
    }

    // fields

    private final String currencyId;
    private final String name;
    private final String code;
    private final String initialSupply;
    private final String totalSupply;
    private final String blockchainId;
    private final String address;
    private final String type;
    private final List<CurrencyDenomination> denominations;
    private final Boolean verified;

    private Currency(String currencyId,
                     String name,
                     String code,
                     String initialSupply,
                     String totalSupply,
                     String type,
                     String blockchainId,
                     String address,
                     Boolean verified,
                     List<CurrencyDenomination> denominations) {
        this.currencyId = currencyId;
        this.name = name;
        this.code = code;
        this.initialSupply = initialSupply;
        this.totalSupply = totalSupply;
        this.type = type;
        this.blockchainId = blockchainId;
        this.address = address;
        this.verified = verified;
        this.denominations = denominations;
    }

    // getters

    @JsonProperty("currency_id")
    public String getId() {
        return currencyId;
    }

    @JsonProperty("name")
    public String getName() {
        return name;
    }

    @JsonProperty("code")
    public String getCode() {
        return code;
    }

    @JsonProperty("initial_supply")
    public String getInitialSupply() {
        return initialSupply;
    }

    @JsonProperty("total_supply")
    public String getTotalSupply() {
        return totalSupply;
    }

    @JsonProperty("blockchain_id")
    public String getBlockchainId() {
        return blockchainId;
    }

    @JsonProperty("type")
    public String getType() {
        return type;
    }

    @JsonProperty("denominations")
    public List<CurrencyDenomination> getDenominations() {
        return denominations;
    }

    @JsonProperty("verified")
    public boolean getVerified() {
        return verified;
    }

    @JsonProperty("address")
    public String getAddressValue() {
        return address;
    }

    @JsonIgnore
    public Optional<String> getAddress() {
        return ADDRESS_INTERNAL.equals(address) ? Optional.absent() : Optional.fromNullable(address);
    }
}
