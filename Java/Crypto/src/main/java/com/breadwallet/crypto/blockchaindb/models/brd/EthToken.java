/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class EthToken {

    public static Optional<EthToken> asToken(JSONObject json) {
        try {
            String name = json.getString("name");
            String symbol = json.getString("code");
            String address = json.getString("contract_address");
            UnsignedInteger decimals = UnsignedInteger.valueOf(json.getLong("scale"));
            String description = String.format("Token for %s", symbol);

            return Optional.of(new EthToken(address, symbol, name, description, decimals, null, null));
        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    public static Optional<List<EthToken>> asTokens(JSONArray json) {
        List<EthToken> objs = new ArrayList<>();
        for (int i = 0; i < json.length(); i++) {
            JSONObject obj = json.optJSONObject(i);
            if (obj == null) {
                return Optional.absent();
            }

            Optional<EthToken> opt = EthToken.asToken(obj);
            if (!opt.isPresent()) {
                return Optional.absent();
            }

            objs.add(opt.get());
        }
        return Optional.of(objs);
    }

    private final String address;
    private final String symbol;
    private final String name;
    private final String description;
    private final UnsignedInteger decimals;

    @Nullable
    private final String defaultGasLimit;
    @Nullable
    private final String defaultGasPrice;

    private EthToken(String address, String symbol, String name, String description, UnsignedInteger decimals,
                     String defaultGasLimit, @Nullable String defaultGasPrice) {
        this.address = address;
        this.symbol = symbol;
        this.name = name;
        this.description = description;
        this.decimals = decimals;
        this.defaultGasLimit = defaultGasLimit;
        this.defaultGasPrice = defaultGasPrice;
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
        return description;
    }

    public UnsignedInteger getDecimals() {
        return decimals;
    }

    public Optional<String> getDefaultGasLimit() {
        return Optional.fromNullable(defaultGasLimit);
    }

    public Optional<String> getDefaultGasPrice() {
        return Optional.fromNullable(defaultGasPrice);
    }
}
