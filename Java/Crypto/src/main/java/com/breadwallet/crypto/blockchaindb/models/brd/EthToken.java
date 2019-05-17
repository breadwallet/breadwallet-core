package com.breadwallet.crypto.blockchaindb.models.brd;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;

import org.json.JSONException;
import org.json.JSONObject;

public class EthToken {

    public static Optional<EthToken> asToken(JSONObject json, int rid) {
        try {
            String name = json.getString("name");
            String symbol = json.getString("code");
            String address = json.getString("contract_address");
            int decimals = json.getInt("scale");
            String description = String.format("Token for %s", symbol);

            return Optional.of(new EthToken(address, symbol, name, description, decimals, null, null, rid));
        } catch (JSONException e) {
            return Optional.absent();
        }
    }

    private final String address;
    private final String symbol;
    private final String name;
    private final String description;
    private final int decimals;
    private final int rid;

    @Nullable
    private final String defaultGasLimit;
    @Nullable
    private final String defaultGasPrice;

    private EthToken(String address, String symbol, String name, String description, int decimals,
                     String defaultGasLimit, @Nullable String defaultGasPrice, int rid) {
        this.address = address;
        this.symbol = symbol;
        this.name = name;
        this.description = description;
        this.decimals = decimals;
        this.defaultGasLimit = defaultGasLimit;
        this.defaultGasPrice = defaultGasPrice;
        this.rid = rid;
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

    public int getDecimals() {
        return decimals;
    }

    public int getRid() {
        return rid;
    }

    public Optional<String> getDefaultGasLimit() {
        return Optional.fromNullable(defaultGasLimit);
    }

    public Optional<String> getDefaultGasPrice() {
        return Optional.fromNullable(defaultGasPrice);
    }
}
