/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;

import static com.google.common.base.Preconditions.checkNotNull;

public class Blockchain {

    private static final UnsignedLong BLOCK_HEIGHT_UNSPECIFIED = UnsignedLong.MAX_VALUE;

    // creators

    @JsonCreator
    public static Blockchain create(@JsonProperty("id") String id,
                                    @JsonProperty("name") String name,
                                    @JsonProperty("network") String network,
                                    @JsonProperty("is_mainnet") Boolean isMainNet,
                                    @JsonProperty("native_currency_id") String currencyId,
                                    @JsonProperty("block_height") UnsignedLong blockHeight,
                                    @JsonProperty("fee_estimates") List<BlockchainFee> feeEstimates,
                                    @JsonProperty("confirmations_until_final") UnsignedInteger confirmationsUntilFinal) {
        return new Blockchain(
                checkNotNull(id),
                checkNotNull(name),
                checkNotNull(network),
                checkNotNull(isMainNet),
                checkNotNull(currencyId),
                checkNotNull(blockHeight),
                checkNotNull(feeEstimates),
                checkNotNull(confirmationsUntilFinal)
        );
    }

    // fields

    private final String name;
    private final String id;
    private final String currencyId;
    private final List<BlockchainFee> feeEstimates;
    private final Boolean isMainNet;
    private final String network;
    private final UnsignedLong blockHeight;
    private final UnsignedInteger confirmationsUntilFinal;

    private Blockchain(String id,
                       String name,
                       String network,
                       boolean isMainNet,
                       String currencyId,
                       UnsignedLong blockHeight,
                       List<BlockchainFee> feeEstimates,
                       UnsignedInteger confirmationsUntilFinal) {
        this.id = id;
        this.name = name;
        this.network = network;
        this.isMainNet = isMainNet;
        this.currencyId = currencyId;
        this.blockHeight = blockHeight;
        this.feeEstimates = feeEstimates;
        this.confirmationsUntilFinal = confirmationsUntilFinal;
    }

    // getters

    @JsonProperty("name")
    public String getName() {
        return name;
    }

    @JsonProperty("id")
    public String getId() {
        return id;
    }

    @JsonProperty("native_currency_id")
    public String getCurrency() {
        return currencyId;
    }

    @JsonProperty("fee_estimates")
    public List<BlockchainFee> getFeeEstimates() {
        return feeEstimates;
    }

    @JsonProperty("is_mainnet")
    public boolean isMainnet() {
        return isMainNet;
    }

    @JsonProperty("network")
    public String getNetwork() {
        return network;
    }

    @JsonProperty("confirmations_until_final")
    public UnsignedInteger getConfirmationsUntilFinal() {
        return confirmationsUntilFinal;
    }

    @JsonProperty("block_height")
    public UnsignedLong getBlockHeightValue() {
        return blockHeight;
    }

    @JsonIgnore
    public Optional<UnsignedLong> getBlockHeight() {
        return BLOCK_HEIGHT_UNSPECIFIED.equals(blockHeight) ? Optional.absent() : Optional.of(blockHeight);
    }
}
