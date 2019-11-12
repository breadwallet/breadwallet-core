/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.brd;

import android.support.annotation.Nullable;

import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.gson.annotations.SerializedName;

import static com.google.common.base.Preconditions.checkNotNull;

public class EthToken {

    @SerializedName("contract_address")
    private String address;

    @SerializedName("code")
    private String symbol;

    private String name;

    @SerializedName("scale")
    private Long decimals;

    // TODO(fix): defaultGasLimit and defaultGasPrice are not present in the JSON response

    @Nullable
    private String defaultGasLimit;

    @Nullable
    private String defaultGasPrice;

    public String getAddress() {
        checkNotNull(address);
        return address;
    }

    public String getSymbol() {
        checkNotNull(symbol);
        return symbol;
    }

    public String getName() {
        checkNotNull(name);
        return name;
    }

    public String getDescription() {
        return String.format("Token for %s", getSymbol());
    }

    public UnsignedInteger getDecimals() {
        checkNotNull(decimals);
        return UnsignedInteger.valueOf(decimals);
    }

    public Optional<String> getDefaultGasLimit() {
        return Optional.fromNullable(defaultGasLimit);
    }

    public Optional<String> getDefaultGasPrice() {
        return Optional.fromNullable(defaultGasPrice);
    }
}
