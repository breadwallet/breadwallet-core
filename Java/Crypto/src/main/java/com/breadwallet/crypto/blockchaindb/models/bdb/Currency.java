/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import android.support.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;

import java.util.List;

public class Currency {

    private static final String ADDRESS_INTERNAL = "__native__";

    // creators

    public static Currency create(String currencyId,
                                  String name,
                                  String code,
                                  String type,
                                  String blockchainId,
                                  @Nullable String address,
                                  boolean verified,
                                  List<CurrencyDenomination> denominations) {
        Currency currency = new Currency();
        currency.currencyId = currencyId;
        currency.name = name;
        currency.code = code;
        currency.type = type;
        currency.blockchainId = blockchainId;
        currency.address = address;
        currency.verified = verified;
        currency.denominations = denominations;
        return currency;
    }

    // fields

    @JsonProperty("currency_id")
    private String currencyId;

    @JsonProperty("name")
    private String name;

    @JsonProperty("code")
    private String code;

    @JsonProperty("initial_supply")
    private String initialSupply;

    @JsonProperty("total_supply")
    private String totalSupply;

    @JsonProperty("blockchain_id")
    private String blockchainId;

    @JsonProperty("address")
    private String address;

    @JsonProperty("type")
    private String type;

    @JsonProperty("denominations")
    private List<CurrencyDenomination> denominations;

    @JsonProperty("verified")
    private Boolean verified;

    // getters

    public String getId() {
        return currencyId;
    }

    public String getName() {
        return name;
    }

    public String getCode() {
        return code;
    }

    public String getInitialSupply() {
        return initialSupply;
    }

    public String getTotalSupply() {
        return totalSupply;
    }

    public String getBlockchainId() {
        return blockchainId;
    }

    public Optional<String> getAddress() {
        return ADDRESS_INTERNAL.equals(address) ? Optional.absent() : Optional.fromNullable(address);
    }

    public String getType() {
        return type;
    }

    public List<CurrencyDenomination> getDenominations() {
        return denominations;
    }

    public Boolean getVerified() {
        return verified;
    }
}
