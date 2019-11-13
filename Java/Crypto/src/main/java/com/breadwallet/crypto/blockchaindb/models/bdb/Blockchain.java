/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.crypto.blockchaindb.models.bdb;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.google.common.primitives.UnsignedLong;

import java.util.List;

public class Blockchain {

    private static final long BLOCK_HEIGHT_UNSPECIFIED = -1;

    // creators

    public static Blockchain create(String id,
                                    String name,
                                    String network,
                                    boolean isMainNet,
                                    String currencyId,
                                    UnsignedLong blockHeight,
                                    List<BlockchainFee> feeEstimates,
                                    UnsignedInteger confirmationsUntilFinal) {
        Blockchain blockchain = new Blockchain();
        blockchain.id = id;
        blockchain.name = name;
        blockchain.network = network;
        blockchain.isMainNet = isMainNet;
        blockchain.currencyId = currencyId;
        blockchain.blockHeight = blockHeight;
        blockchain.feeEstimates = feeEstimates;
        blockchain.confirmationsUntilFinal = confirmationsUntilFinal;
        return blockchain;
    }

    // fields

    public String name;
    @JsonProperty("id")
    public String id;
    @JsonProperty("native_currency_id")
    public String currencyId;
    @JsonProperty("fee_estimates")
    public List<BlockchainFee> feeEstimates;
    @JsonProperty("is_mainnet")
    public Boolean isMainNet;
    public String network;
    @JsonProperty("block_height")
    public UnsignedLong blockHeight;
    @JsonProperty("confirmations_until_final")
    public UnsignedInteger confirmationsUntilFinal;

    // getters

    public String getName() {
        return name;
    }

    public String getId() {
        return id;
    }

    public String getCurrency() {
        return currencyId;
    }

    public List<BlockchainFee> getFeeEstimates() {
        return feeEstimates;
    }

    public boolean isMainnet() {
        return isMainNet;
    }

    public String getNetwork() {
        return network;
    }

    public Optional<UnsignedLong> getBlockHeight() {
        return blockHeight.longValue() == BLOCK_HEIGHT_UNSPECIFIED ? Optional.absent() : Optional.fromNullable(blockHeight);
    }

    public UnsignedInteger getConfirmationsUntilFinal() {
        return confirmationsUntilFinal;
    }
}
